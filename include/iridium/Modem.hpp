#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <stdint.h>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

namespace Iridium {

class Modem: private boost::noncopyable
{
  public:
    enum EClearMessageBuffers: uint8_t
    {
      eClearMObuffer = 0,
      eClearMTbuffer = 1,
      eClearBoth = 2
    };

    struct SbdStatus
    {
      bool mo_flag; ///< True, if message in mobile originated buffer.
      uint16_t momsn; ///< The sequence number that will be used during the
                      ///< next mobile originated SBD session.
      bool mt_flag; ///< True if message in mobile terminated buffer.
      int32_t mtmsn; ///< The sequence number that was used in the most recent
                     ///< mobile terminated SBD session. This value will be 
                     ///< if there is nothing in the mobile terminated buffer.
      bool ra_flag; ///< True, if SBD ring alert has been received and not
                    ///< answered.
      uint8_t mt_queue; ///< How many SBD mobile terminated messages are
                        ///< currently queued at the gateway, up to 50.

      SbdStatus():
        mo_flag(false), momsn(0), mt_flag(false), mtmsn(-1), ra_flag(false),
        mt_queue(0)
      {}
    };
 
    struct SbdSessionStatus
    {
      uint8_t mo_status; ///< An indication of the disposition of the mobile
                         ///< originated transaction.
      uint16_t momsn; ///< A value assigned by the ISU to last
                      ///< mobile-originated message sended to the GSS.
      uint8_t mt_status; ///< An indication of the disposition of the mobile
                         ///< terminated transaction.
      uint16_t mtmsn; ///< Assigned by the GSS when forwarding a message to the
                      ///< ISU. This value is indeterminate if @mt_status is
                      ///< zero.
      uint16_t mt_message_len; ///< The length in bytes of the mobile
                               ///< terminated SBD message received from the
                               ///< GSS. If no message was received, this field
                               ///< will be zero.
      uint8_t mt_queue; ///< A count of mobile terminated SBD messages waiting
                        ///< at the GSS.
    };

    Modem(boost::asio::io_service& service, const std::string& device);
    ~Modem();

    inline bool isOpen() const { return m_io.is_open(); }

    ///
    /// @throw std::runtime_error
    ///
    void Open();
    void Close();

    bool NetGetStatus(uint8_t& status);
    bool GetSignalQuality(uint8_t& quality, bool lastKnown = false);
    bool SbdDetach(uint8_t& error);
    bool SbdGetStatus(SbdStatus& status);
    bool SbdClearBuffers(EClearMessageBuffers clear);
    bool ClearMomsn();
    ///
    /// Write MO message to modem buffer.
    ///
    /// @param [in] payload Payload buffer.
    /// @return Result of operation.
    /// @throw std::runtime_error
    ///
    /// Result codes:
    /// 0 - SBD message successfully written to the modem.
    /// 1 - SBD message write timeout. An insufficient number of bytes were
    ///     transferred to modem during the transfer period of 60 seconds.
    /// 2 - SBD message checksum sent from DTE does not match the checksum
    ///     calculated at the modem.
    /// 3 - SBD message size is not correct. The maximum mobile originated SBD
    ///     message length is 1960 bytes, the minimum is 1 byte.
    ///
    uint8_t WriteMessage(const std::vector<char>& payload);
    ///
    /// Read MT message from modem buffer.
    ///
    /// @param [out] payload Payload buffer.
    ///
    /// Payload buffer is empty, if no message in modem buffer or Modem not
    /// opened.
    ///
    void ReadMessage(std::vector<char>& payload);
    ///
    /// @param [in] answer True, if the SBD session is in response to an SBD
    ///                    ring alert.
    /// @throw std::runtime_error
    ///
    SbdSessionStatus DoSbdSession(bool answer);

  private:
    ///
    /// Читать данные из последовательного порта до обнаружения разделителя.
    ///
    /// @param [out] buf Буфер полученных данных, включая разделитель.
    /// @param [in] delim Разделитель данных.
    /// @param [out] ec Код завершения операции.
    /// @param [in] timeout Таймаут операции.
    ///
    /// По истечению таймаута операция прерывается и считается завершившейся
    /// неудачно.
    ///
    /// Метод есть синхронный интерфейс к асинхронными операциями ввода. Ввод
    /// выполняется в отдельном экземпляре boost::asio::io_service, в нем же
    /// работает таймер. Цикл ввода/вывода исполняется в отдельном потоке,
    /// запускаемом и останавливаемом в методе.
    ///
    void read_until_timeout(boost::asio::streambuf& buf,
                            const std::string& delim,
                            boost::system::error_code& ec, uint16_t timeout);

    boost::asio::io_service& m_service; ///< Внешний (глобальный) сервис
                                        ///< ввода/вывода; используется только
                                        ///< для указания через "сторожа"
                                        ///< @m_sentinel, что экземпляр данного
                                        ///< класса не закончил операции
                                        ///< ввода/вывода.
    std::shared_ptr<boost::asio::io_service::work>
      m_sentinel; ///< "Сторож" для внешнего цикла ввода/вывода.
    boost::asio::io_service m_ioService; ///< Экземпляр службы ввода/вывода
                                         ///< для асинхронного чтения из
                                         ///< последовательного порта.
    boost::asio::serial_port m_io; ///< Ввод/вывод последовательного порта,
                                   ///< использует m_ioService.
    boost::asio::steady_timer m_timer; ///< Таймер для асинхронного чтения из
                                       ///< порта, использует m_ioService.
    std::mutex m_mutex; ///< Критическая секция -- чтение из порта.
    std::string m_deviceName; ///< Наименование устройства в файловой системе,
                              ///< соответствующего последовательному порту.
};

}; // namespace Iridium
