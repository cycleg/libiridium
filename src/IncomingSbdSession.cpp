#include <cstring>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <netinet/in.h>
#include "iridium/Codec.hpp"
#include "iridium/SbdReceiver.hpp"
#include "iridium/IncomingSbdSession.hpp"

using namespace Iridium;

IncomingSbdSession::IncomingSbdSession(boost::asio::ip::tcp::socket socket,
                                       std::shared_ptr<SbdReceiver>& receiver):
  m_socket(std::move(socket)),
  m_receiver(receiver),
  m_messageLength(0)
{
}

void IncomingSbdSession::run()
{
  auto self(shared_from_this()); // syntax closure
  m_socket.async_read_some(
    m_inBuf.prepare(SbdDirectIp::MoMessage::MaxMessageSize +
                    sizeof(SbdDirectIp::MessageHeader)),
    [this, self](boost::system::error_code ec, std::size_t bytes) {
      onRead(ec, bytes);
    }
  );
}

void IncomingSbdSession::onRead(const boost::system::error_code& ec,
                                std::size_t bytes)
{
  if (ec)
  {
    if (!m_receiver.expired())
    {
      auto rcv = m_receiver.lock();
      rcv->m_OnError(ec.message());
    }
    return;
  }
  m_inBuf.commit(bytes);
  std::string buf;
  if ((m_inBuf.size() >= sizeof(SbdDirectIp::MessageHeader)) && !m_messageLength)
  {
    SbdDirectIp::MessageHeader header;
    buf.assign(
      boost::asio::buffers_begin(m_inBuf.data()),
      boost::asio::buffers_begin(m_inBuf.data()) + sizeof(SbdDirectIp::MessageHeader)
    );
    std::memcpy(&header, buf.data(), sizeof(SbdDirectIp::MessageHeader));
    m_inBuf.consume(sizeof(SbdDirectIp::MessageHeader));
    header.m_length = ntohs(header.m_length);
    if (header.m_proto != SbdDirectIp::SbdProtoNumber)
    {
      if (!m_receiver.expired())
      {
        auto rcv = m_receiver.lock();
        std::ostringstream err;
        err << "invalid  protocol number " << int(header.m_proto);
        rcv->m_OnError(err.str());
      }
      return;
    }
    m_messageLength = header.m_length;
  }
  if (m_inBuf.size() < m_messageLength)
  {
    // read rest of message
    auto self(shared_from_this());
    m_socket.async_read_some(
      m_inBuf.prepare(SbdDirectIp::MoMessage::MaxMessageSize),
      [this, self](boost::system::error_code ec, std::size_t bytes) {
        onRead(ec, bytes);
      }
    );
    return;
  }
  buf.clear();
  buf.assign(boost::asio::buffers_begin(m_inBuf.data()),
             boost::asio::buffers_begin(m_inBuf.data()) + m_messageLength);
  SbdDirectIp::Codec::EMessageCategory msgCategory =
    SbdDirectIp::Codec::messageCategory(buf.data(), m_messageLength);
  if (msgCategory != SbdDirectIp::Codec::eMoMessage)
  {
    if (!m_receiver.expired())
    {
      auto rcv = m_receiver.lock();
      std::ostringstream err;
      if (msgCategory != SbdDirectIp::Codec::eUnknownMessage)
        err << "unexpected ";
      err << SbdDirectIp::Codec::categoryStr(msgCategory);
      rcv->m_OnError(err.str());
    }
    return;
  }
  SbdDirectIp::MoMessage message;
  try
  {
    SbdDirectIp::Codec::parse(buf.data(), m_messageLength, message);
  }
  catch (std::runtime_error& e)
  {
    if (!m_receiver.expired())
    {
      auto rcv = m_receiver.lock();
      std::ostringstream err;
      err << "message parse error: " << e.what();
      rcv->m_OnError(err.str());
    }
    return;
  }
  m_inBuf.consume(m_messageLength);
  if (!m_receiver.expired())
  {
    auto rcv = m_receiver.lock();
    rcv->m_OnMessage(message);
    if (m_inBuf.size() > 0)
    {
      // для доставки очередного сообщения "Иридиум" откроет новую сессию
      std::ostringstream err;
      err << "unexpected " << m_inBuf.size()  << " bytes received";
      rcv->m_OnError(err.str());
    }
  }
}
