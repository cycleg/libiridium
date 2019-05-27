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
    ///
    /// Incoming data callback.
    ///
    /// @param [in] ec Async read result code.
    /// @param [in] bytes Incoming data size.
    ///
    /// Parse incoming data.
    ///
    void onRead(const boost::system::error_code& ec, std::size_t bytes);

    boost::asio::ip::tcp::socket m_socket; ///< Incomig connection socket.
    boost::asio::streambuf m_inBuf; ///< Async operations buffer.
    uint16_t m_messageLength; ///< Length from MO header information element.
}; // class IncomingSbdSession

}; // namespace Iridium
