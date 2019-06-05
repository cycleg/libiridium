#include <chrono>
#include <cstring>
#include <functional>
#include <iostream>
#include <netinet/in.h>
#include "iridium/Codec.hpp"
#include "iridium/SbdTransmitter.hpp"

using namespace Iridium;

const unsigned short int SbdTransmitter::Heartbeat = 100;
const unsigned short int SbdTransmitter::MaxDelay = 64;

SbdTransmitter::SbdTransmitter(boost::asio::io_service& service,
                               const std::string& host, const std::string& port):
  m_service(service),
  m_delayTimer(m_service),
  m_host(host),
  m_port(port),
  m_socket(m_service),
  m_resolver(m_service),
  m_running(false),
  m_shutdown(false),
  m_errDelay(1),
  m_prevState(eNotConnected),
  m_state(eNotConnected),
  m_confirmationLength(0)
{
}

SbdTransmitter::~SbdTransmitter()
{
  stop(true);
}

void SbdTransmitter::start()
{
  if (m_running) return;
  m_running = true;
  m_shutdown = false;
  auto thread = std::make_shared<std::thread>(
    std::bind(&SbdTransmitter::worker, this)
  );
  m_thread.swap(thread);
}

void SbdTransmitter::stop(bool woexcept)
{
  if (!m_running) return;
  m_shutdown = true;
  try
  {
    m_delayTimer.cancel();
  }
  catch (...)
  {
    if (!woexcept) throw;
  }
  try
  {
    if (m_socket.is_open()) m_socket.cancel();
  }
  catch (...)
  {
    if (!woexcept) throw;
  }
  try
  {
    m_thread->join();
  }
  catch (...)
  {
    if (!woexcept) throw;
  }
}

void SbdTransmitter::post(const SbdDirectIp::MtMessage& message)
{
  m_messageQueue.put(message);
  m_messageQueue.notify_one();
}

void SbdTransmitter::worker()
{
  m_errDelay = 1;
  m_prevState = eNotConnected;
  m_state = eNotConnected;
  while (!m_shutdown)
  {
    if (m_state == eNotConnected)
      {
        if (m_messageQueue.wait_for(std::chrono::milliseconds(Heartbeat)))
          continue;
        m_prevState = m_state;
        m_state = eResolving;
        StateMachine();
      }
      else
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(Heartbeat));
      }
  }
  m_sentinel.reset();
  m_running = false;
}

void SbdTransmitter::closeSocket()
{
  boost::system::error_code ec;
  boost::asio::ip::tcp::endpoint endpoint = m_socket.remote_endpoint(ec);
  (void)endpoint;
  try
  {
    // if connection close by other side, the underlying descriptor
    // already closed
    if (!ec) m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
  }
  catch (boost::system::system_error& e)
  {
    // socket already dead
  }
  try
  {
    if (!ec) m_socket.close();
  }
  catch (boost::system::system_error& e)
  {
    // ignore error
  }
}

void SbdTransmitter::StateMachine()
{
  switch (m_state)
  {
    case eNotConnected:
std::clog << "SbdTransmitter::StateMachine() " << " messages " << m_messageQueue.size() << std::endl;
      m_buf.reset();
      m_confirmationLength = 0;
      m_sentinel.reset();
      break;
    case eResolving:
      // Iridium SBD service developer guide, p. 7.2.1 "MT Vendor Client Requirements"
      // Step A.
      {
        auto work = std::make_shared<boost::asio::io_service::work>(m_service);
        m_sentinel.swap(work);
      }
      m_resolver.async_resolve(boost::asio::ip::tcp::resolver::query(m_host, m_port),
                               [this](const boost::system::error_code& ec,
                                      boost::asio::ip::tcp::resolver::iterator i) {
        // something happened...
        if (ec == boost::asio::error::operation_aborted) return;
        if ((m_state != eResolving) || m_shutdown) return;
        if (ec) {
          std::cerr << "SbdTransmitter::StateMachine() failed to resolve " << m_host
                    << ":" << m_port << ": " << ec.message() << std::endl;
          m_prevState = m_state;
          m_state = eError;
          StateMachine();
          return;
        }
        m_rIterator = i;
        m_prevState = m_state;
        m_state = eConnecting;
        StateMachine();
      });
      break;
    case eConnecting:
      boost::asio::async_connect(m_socket, m_rIterator,
                                 [this](const boost::system::error_code& ec,
                                        boost::asio::ip::tcp::resolver::iterator i) {
        if (ec == boost::asio::error::operation_aborted) return;
        if ((m_state != eConnecting) || m_shutdown) return;
        if (ec || (i == boost::asio::ip::tcp::resolver::iterator()))
        {
          std::cerr << "SbdTransmitter::StateMachine() connection error: " << ec.message()
                    << std::endl;
          m_prevState = m_state;
          m_state = eError;
          StateMachine();
          return;
        }
        m_prevState = m_state;
        m_state = eSending;
        StateMachine();
      });
      break;
    case eSending:
      // Step B.
      m_sendingMessage = m_messageQueue.get();
      m_buf = std::make_shared<boost::asio::streambuf>();
      {
        std::vector<char> buf(std::move(m_sendingMessage.serialize()));
        std::ostream os(m_buf.get());
        os.write(buf.data(), buf.size());
      }
      boost::asio::async_write(
        m_socket, *m_buf.get(),
        [this](const boost::system::error_code& ec, std::size_t bytes) {
          (void)bytes;
          if (ec == boost::asio::error::operation_aborted) return;
          m_buf.reset();
          if ((m_state != eSending) || m_shutdown) return;
          if (ec)
          {
            std::cerr << "SbdTransmitter::StateMachine() transmit error: " << ec.message()
                      << std::endl;
            m_prevState = m_state;
            m_state = eError;
            StateMachine();
            return;
          }
          m_prevState = m_state;
          m_state = eReceivingHeader;
          StateMachine();
      });
      break;
    case eReceivingHeader:
      m_buf.reset();
      m_buf = std::make_shared<boost::asio::streambuf>();
      m_socket.async_read_some(
        m_buf->prepare(SbdDirectIp::MoMessage::MaxMessageSize),
        [this](const boost::system::error_code& ec, std::size_t bytes) {
          if (ec == boost::asio::error::operation_aborted) return;
          if ((m_state != eReceivingHeader) || m_shutdown) return;
          if (ec)
          {
            std::cerr << "SbdTransmitter::StateMachine() receive confirmation error: " << ec.message()
                      << std::endl;
            m_prevState = m_state;
            m_state = eError;
            StateMachine();
            return;
          }
          m_buf->commit(bytes);
          SbdDirectIp::MessageHeader header;
          std::string buf;
          buf.assign(
            boost::asio::buffers_begin(m_buf->data()),
            boost::asio::buffers_begin(m_buf->data()) + sizeof(SbdDirectIp::MessageHeader)
          );
          std::memcpy(&header, buf.data(), sizeof(SbdDirectIp::MessageHeader));
          m_buf->consume(sizeof(SbdDirectIp::MessageHeader));
          header.m_length = ntohs(header.m_length);
          if (header.m_proto != SbdDirectIp::SbdProtoNumber)
          {
            std::cerr << "SbdTransmitter::StateMachine() error: protocol number is "
                      << header.m_proto << std::endl;
            m_prevState = m_state;
            m_state = eError;
            StateMachine();
            return;
          }
          m_confirmationLength = header.m_length;
          m_prevState = m_state;
          m_state = eReceivingConfirmation;
          StateMachine();
      });
      break;
    case eReceivingConfirmation:
      if (m_buf->size() < m_confirmationLength)
        {
          m_socket.async_read_some(
            m_buf->prepare(m_confirmationLength),
            [this](const boost::system::error_code& ec, std::size_t bytes) {
              if (ec == boost::asio::error::operation_aborted) return;
              if ((m_state != eReceivingConfirmation) || m_shutdown) return;
              if (ec)
              {
                std::cerr << "SbdTransmitter::StateMachine() receive confirmation error: " << ec.message()
                          << std::endl;
                m_prevState = m_state;
                m_state = eError;
                StateMachine();
                return;
              }
              m_buf->commit(bytes);
              m_prevState = m_state;
              m_state = eReceivingConfirmation;
              StateMachine();
          });
        }
        else
        {
          m_prevState = m_state;
          m_state = eProcessingConfirmation;
          StateMachine();
        }
      break;
    case eProcessingConfirmation:
      {
        std::string buf;
        buf.assign(boost::asio::buffers_begin(m_buf->data()),
                   boost::asio::buffers_begin(m_buf->data()) + m_confirmationLength);
        SbdDirectIp::Codec::EMessageCategory msgCategory =
          SbdDirectIp::Codec::messageCategory(buf.data(), m_confirmationLength);
        if (msgCategory != SbdDirectIp::Codec::eMtConfirmMessage)
        {
          std::cerr << "SbdTransmitter::StateMachine() receive confirmation error: ";
          if (msgCategory != SbdDirectIp::Codec::eUnknownMessage)
            std::cerr << "unexpected ";
          std::cerr << SbdDirectIp::Codec::categoryStr(msgCategory) << std::endl;
          m_prevState = m_state;
          m_state = eError;
          StateMachine();
          return;
        }
        SbdDirectIp::MtConfirmMessage confirmation;
        try
        {
          SbdDirectIp::Codec::parse(buf.data(), m_confirmationLength, confirmation);
        }
        catch (std::runtime_error& e)
        {
          std::cerr << "SbdTransmitter::StateMachine() confirmation parse error: "
                    << e.what() << std::endl;
          m_prevState = m_state;
          m_state = eError;
          StateMachine();
          return;
        }
        m_buf->consume(m_confirmationLength);
        std::clog << confirmation << std::endl;
        if (confirmation.status() < 0)
        {
          m_prevState = m_state;
          m_state = eError;
          StateMachine();
          return;
        }
      }
      if (m_buf->size() > 0)
        std::cerr << "SbdTransmitter::StateMachine(): " << m_buf->size()
                  << " unexpected bytes received." << std::endl;
      m_errDelay = 1;
      m_prevState = m_state;
      m_state = eSuccess;
      StateMachine();
      break;
    case eError:
      if (m_prevState >= eSending) m_messageQueue.unget(m_sendingMessage);
      if (m_prevState > eConnecting) closeSocket();
      m_delayTimer.expires_from_now(std::chrono::milliseconds(Heartbeat * m_errDelay));
      m_delayTimer.async_wait([this](const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted) return;
        if (m_errDelay < MaxDelay) m_errDelay *= 2;
        m_prevState = m_state;
        m_state = eNotConnected;
        StateMachine();
      });
      break;
    case eSuccess:
      // Step C.
      closeSocket();
      m_prevState = m_state;
      m_state = eNotConnected;
      StateMachine();
      break;
  }
}
