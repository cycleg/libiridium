#include <cstring>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <netinet/in.h>
#include "iridium/Codec.hpp"
#include "iridium/IncomingSbdSession.hpp"

using namespace Iridium;

IncomingSbdSession::IncomingSbdSession(boost::asio::ip::tcp::socket socket):
  m_socket(std::move(socket)),
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
    std::cerr << "IncomingSbdSession::onRead() error: " << ec.message()
              << std::endl;
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
      std::cerr << "IncomingSbdSession::onRead() error: protocol number is "
                << int(header.m_proto) << std::endl;
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
    std::cerr << "IncomingSbdSession::onRead() error: ";
    if (msgCategory != SbdDirectIp::Codec::eUnknownMessage)
      std::cerr << "unexpected ";
    std::cerr << SbdDirectIp::Codec::categoryStr(msgCategory) << std::endl;
    return;
  }
  SbdDirectIp::MoMessage message;
  try
  {
    SbdDirectIp::Codec::parse(buf.data(), m_messageLength, message);
  }
  catch (std::runtime_error& e)
  {
    std::cerr << "IncomingSbdSession::onRead() message parse error: " << e.what() << std::endl;
    return;
  }
  m_inBuf.consume(m_messageLength);
  std::clog << message << std::endl;
  if (m_inBuf.size() > 0)
    std::cerr << "IncomingSbdSession::onRead(): " << m_inBuf.size()
              << " unexpected bytes received." << std::endl;
}
