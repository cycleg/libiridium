#pragma once

#include <memory>
#include <stdint.h>
#include <boost/asio.hpp>
#include "Message.hpp"

namespace Iridium {

class IncomingSbdSession: public std::enable_shared_from_this<IncomingSbdSession>
{
  public:
    IncomingSbdSession(boost::asio::ip::tcp::socket socket);

    void run();

  private:
    void onRead(const boost::system::error_code& ec, std::size_t bytes);

    boost::asio::ip::tcp::socket m_socket;
    boost::asio::streambuf m_inBuf;
    uint16_t m_messageLength;
}; // class IncomingSbdSession

}; // namespace Iridium
