#pragma once

#include <memory>
#include <string>
#include <boost/asio.hpp>
#include <boost/signals2/signal.hpp>
#include "iridium/Message.hpp"
#include "iridium/IncomingSbdSession.hpp"

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
class SbdReceiver: public std::enable_shared_from_this<SbdReceiver>
{
  friend void IncomingSbdSession::onRead(const boost::system::error_code& ec,
                                         std::size_t bytes);

  public:
    typedef std::shared_ptr<SbdReceiver> Pointer;

    typedef boost::signals2::signal<void (const std::string&)> SignalOnError;
    typedef boost::signals2::signal<void (const SbdDirectIp::MoMessage&)> SignalOnMessage;

    ~SbdReceiver();

    static Pointer Factory(boost::asio::io_service& service, short int port);
    static Pointer Factory(boost::asio::io_service& service,
                           const boost::asio::ip::tcp::socket::endpoint_type& endpoint);

    inline boost::signals2::connection OnErrorConnect(
      const SignalOnError::slot_type& subscriber
    )
    {
      return m_OnError.connect(subscriber);
    }
    inline boost::signals2::connection OnMessageConnect(
      const SignalOnMessage::slot_type& subscriber
    )
    {
      return m_OnMessage.connect(subscriber);
    }
    inline std::shared_ptr<SbdReceiver> GetPtr() { return shared_from_this(); }

    ///
    /// Открыть принимающий сокет.
    ///
    /// @throw std::runtime_error
    ///
    void start();
    ///
    /// Закрыть принимающий сокет.
    ///
    /// @param [in] woexcept Не поднимать исключения.
    ///
    /// @throw std::runtime_error
    ///
    void stop(bool woexcept = false);

  private:
    SbdReceiver(boost::asio::io_service& service, short int port);
    SbdReceiver(boost::asio::io_service& service,
                const boost::asio::ip::tcp::socket::endpoint_type& endpoint);

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
    SignalOnError m_OnError; ///< Сигнал о возникшей ошибке приема.
    SignalOnMessage m_OnMessage; ///< Сигнал о входящем сообщении.
}; // class SbdReceiver

} // namespcae Iridium
