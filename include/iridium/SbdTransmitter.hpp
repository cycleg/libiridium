#pragma once

#include <memory>
#include <string>
#include <thread>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include "JobUnitQueue.hpp"
#include "Message.hpp"

namespace Iridium {

///
/// Класс-передатчик SBD-сообщений через DirectIP.
///
/// Сообщения отправляются асинхронно в отдельном потоке в порядке
/// поступления. Передатчик запускается методом start(), в котором создается
/// поток исполнения. Останавливается передатчик методом stop(). Экземпляр
/// передатчика можно запускать и останавливать без ограничений.
///
/// Чтобы поместить сообщение в очередь на отправку используется метод post().
/// Для формирования сообщений используется метод factory() класса Codec. В
/// случае неудачной отправки сообщение возвращается в начало очереди.
///
/// Очередь сообщений можно опустошить вызовом dropMessages().
///
class SbdTransmitter
{
  public:
    SbdTransmitter(boost::asio::io_service& service, const std::string& host,
                   const std::string& port);
    ~SbdTransmitter();

    inline void dropMessages() { m_messageQueue.clear(); }

    void start();
    void stop();
    void post(const SbdDirectIp::MtMessage& message);

  private:
    static const unsigned short int Heartbeat; // ms
    static const unsigned short int MaxDelay;

    ///
    /// Состояния конечного автомата при передаче SBD.
    ///
    enum State
    {
      eNotConnected, ///< Соединение отсутствует.
      eResolving, ///< Разрешается DNS-имя сервера "Иридиума".
      eConnecting, ///< Устанавливается TCP-соединение с сервером.
      eSending, ///< На сервер передается MT-сообщение.
      eReceivingHeader, ///< Принимается заголовок отклика сервера.
      eReceivingConfirmation, ///< Принимается отклик сервера.
      eProcessingConfirmation, ///< Отклик обрабатывается.
      eError, ///< Обработка ошибок во всех прочих состояниях.
      eSuccess ///< Отправка завершилась успешно.
    };

    void worker();
    void closeSocket();
    void logMessage(SbdDirectIp::MtConfirmMessage& message);
    void StateMachine();

    boost::asio::io_service& m_service;
    std::shared_ptr<boost::asio::io_service::work> m_sentinel;
    boost::asio::steady_timer m_delayTimer;
    std::string m_host, m_port;
    boost::asio::ip::tcp::socket m_socket;
    boost::asio::ip::tcp::resolver m_resolver;
    boost::asio::ip::tcp::resolver::iterator m_rIterator;
    bool m_running, m_shutdown;
    unsigned short int m_errDelay;
    std::shared_ptr<std::thread> m_thread;
    JobUnitQueue<SbdDirectIp::MtMessage> m_messageQueue;
    State m_prevState, m_state;
    SbdDirectIp::MtMessage m_sendingMessage;
    std::shared_ptr<boost::asio::streambuf> m_buf;
    SbdDirectIp::ContentLength m_confirmationLength;
}; // class SbdTransmitter

}  // namespace Iridium
