#pragma once

#include <memory>
#include <boost/asio.hpp>

namespace Iridium {

///
/// Класс-приемник SBD-сообщений через DirectIP
///
/// Экземпляр класса открывает на прием сокет на указанном TCP-порту на всех
/// сетевых интерфейсах или на указанной комбинации "адрес-порт". При появлении
/// входящего соединения принимает его, создает экземпляр IncomingSbdSession,
/// которому передает вновь открытый сокет и запускает сессию вызовом ее
/// метода run().
///
/// Открытие принимающего сокета производится асинхронно в start(), соединения
/// принимаются асинхронно. Сокет закрывается в stop().
///
class SbdReceiver
{
  public:
    SbdReceiver(boost::asio::io_service& service, short int port);
    SbdReceiver(boost::asio::io_service& service,
                const boost::asio::ip::tcp::socket::endpoint_type& endpoint);
    ~SbdReceiver();

    ///
    /// Открыть принимающий сокет.
    ///
    /// @throw std::runtime_error
    ///
    void start();
    ///
    /// Закрыть принимающий сокет.
    ///
    /// @throw std::runtime_error
    ///
    void stop();

  private:
    ///
    /// Принять входящее соединение.
    ///
    void doAccept();

    boost::asio::io_service& m_service; ///< Цикл ввода/вывода, в котором
                                        ///< исполняются все операции
                                        ///< ввода/вывода.
    std::shared_ptr<boost::asio::io_service::work>
      m_sentinel; ///< "Сторож", указывающий наличие незавершенных действий
                  ///< ввода/вывода.
    boost::asio::ip::tcp::endpoint m_endpoint;
    boost::asio::ip::tcp::acceptor m_acceptor;
    boost::asio::ip::tcp::socket m_socket; ///< Принимающий сокет.
}; // class SbdReceiver

} // namespcae Iridium
