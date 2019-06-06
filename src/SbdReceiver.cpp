#include <stdexcept>
#include <utility>
#include "iridium/IncomingSbdSession.hpp"
#include "iridium/SbdReceiver.hpp"

using namespace Iridium;

SbdReceiver::SbdReceiver(
  boost::asio::io_service& service,
  short port
):
  m_service(service),
  m_endpoint(boost::asio::ip::tcp::v4(), port),
  m_acceptor(m_service),
  m_socket(m_service)
{
}

SbdReceiver::SbdReceiver(
  boost::asio::io_service& service,
  const boost::asio::ip::tcp::socket::endpoint_type& endpoint
):
  m_service(service),
  m_endpoint(endpoint),
  m_acceptor(m_service),
  m_socket(m_service)
{
}

SbdReceiver::~SbdReceiver()
{
  stop(true);
}

SbdReceiver::Pointer SbdReceiver::Factory(boost::asio::io_service& service,
                                          short int port)
{
  Pointer self(new SbdReceiver(service, port));
  return self;
}

SbdReceiver::Pointer SbdReceiver::Factory(
  boost::asio::io_service& service,
  const boost::asio::ip::tcp::socket::endpoint_type& endpoint
)
{
  Pointer self(new SbdReceiver(service, endpoint));
  return self;
}

void SbdReceiver::start()
{
  if (m_acceptor.is_open()) return;
  boost::system::error_code ec;
  m_acceptor.open(m_endpoint.protocol(), ec);
  if (ec)
  {
    throw std::runtime_error(ec.message().c_str());
  }
  m_acceptor.bind(m_endpoint, ec);
  if (ec)
  {
    try
    {
      m_acceptor.close();
    }
    catch (boost::system::system_error& e)
    {
      // ignore close errors
    }
    throw std::runtime_error(ec.message().c_str());
  }
  m_acceptor.listen(boost::asio::socket_base::max_connections, ec);
  if (ec)
  {
    try
    {
      m_acceptor.close();
    }
    catch (boost::system::system_error& e)
    {
      // ignore close errors
    }
    throw std::runtime_error(ec.message().c_str());
  }
  auto work = std::make_shared<boost::asio::io_service::work>(m_service);
  m_sentinel.swap(work);
  doAccept();
}

void SbdReceiver::stop(bool woexcept)
{
  if (!m_acceptor.is_open()) return;
  boost::system::error_code ec;
  m_sentinel.reset();
  m_acceptor.close(ec);
  if (ec && !woexcept)
  {
    throw std::runtime_error(ec.message().c_str());
  }
}

void SbdReceiver::doAccept()
{
  m_acceptor.async_accept(m_socket,
  [this](boost::system::error_code ec)
  {
    if (ec == boost::asio::error::operation_aborted) return;
    if (!ec)
    {
      auto self(shared_from_this());
      std::make_shared<IncomingSbdSession>(std::move(m_socket), self)->run();
    }
    doAccept();
  });
}
