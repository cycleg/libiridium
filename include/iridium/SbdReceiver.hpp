#pragma once

#include <memory>
#include <boost/asio.hpp>

namespace Iridium {

class SbdReceiver
{
  public:
    SbdReceiver(boost::asio::io_service& service, short int port);
    ~SbdReceiver();

    ///
    /// @throw std::runtime_error
    ///
    void start();
    ///
    /// @throw std::runtime_error
    ///
    void stop();

  private:
    void doAccept();

    boost::asio::io_service& m_service;
    std::shared_ptr<boost::asio::io_service::work> m_sentinel;
    short int m_port; ///< TCP port.
    boost::asio::ip::tcp::endpoint m_endpoint;
    boost::asio::ip::tcp::acceptor m_acceptor;
    boost::asio::ip::tcp::socket m_socket;
}; // class SbdReceiver

}; // namespcae Iridium
