#include <chrono>
#include <iostream>
#include <vector>
#include <time.h>
#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include "iridium/Modem.hpp"
#include "LockFile.hpp"

#define UNUSED(x) (void)x;

const char* modemDevice = "/dev/ttyS0";
// таймаут ожидания сброса lock-файла, миллисекунды
const unsigned int LockFileTimeout = 500;

boost::asio::io_service io_service;
boost::asio::steady_timer mainTimer(io_service);
Iridium::Modem modem(io_service, modemDevice);

void worker(const boost::system::error_code& error)
{
  if (error == boost::asio::error::operation_aborted) return;
  uint8_t netstatus = 0, signalQty = 0;
  time_t rawtime;
  struct tm* timeinfo;
  char buffer[20] = { 0 };
std::clog << "worker() before NetGetStatus()" << std::endl;
std::clog << std::flush;
  modem.NetGetStatus(netstatus);
std::clog << "worker() after NetGetStatus()" << std::endl;
std::clog << std::flush;
  // registered in home network or roaming
  if ((netstatus == 1) || (netstatus == 5)) modem.GetSignalQuality(signalQty);
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(buffer, sizeof(buffer), "%F %T",timeinfo);
  std::cout << buffer << ";" << int(netstatus) << ";" << int(signalQty)
            << std::endl << std::flush;
  std::clog << buffer << ";" << int(netstatus) << ";" << int(signalQty)
            << std::endl << std::flush;
  mainTimer.expires_from_now(std::chrono::seconds(60));
  mainTimer.async_wait(&worker);
}

int main()
{
  bool shutdown = false;
  boost::asio::signal_set stopSignals(io_service, SIGINT, SIGTERM, SIGQUIT);
  stopSignals.async_wait(
  [&](const boost::system::error_code& error, int signal) {
    UNUSED(signal)
    // ignore signal handling cancellation
    if (error == boost::asio::error::operation_aborted) return;
    shutdown = true;
    mainTimer.cancel();
    modem.Close();
  });
  while (!shutdown)
  {
    LockFile lockfile(modemDevice);
    if (lockfile.present())
      std::cout << "Device " << modemDevice
                << " locked by other application, waiting.." << std::flush;
    while (lockfile.present() && !shutdown)
    {
      std::cout << "." << std::flush;
      boost::asio::steady_timer timer(io_service);
      timer.expires_from_now(std::chrono::milliseconds(LockFileTimeout));
      timer.async_wait(
      [](const boost::system::error_code& error) {
        if (error == boost::asio::error::operation_aborted) return;
        io_service.stop();
      });
      io_service.run();
      io_service.reset();
    }
    if (shutdown) continue;
    lockfile.create();
    std::cout << std::endl << "Device " << modemDevice << " acquired."
              << std::endl;
    try
    {
      modem.Open();
    }
    catch (std::runtime_error& e)
    {
      std::cerr << "Can't open modem: " << e.what() << std::endl;
      return EXIT_FAILURE;
    }
#if 0
    uint8_t buf8;
    if (modem.NetGetStatus(buf8))
      std::cout << "Network status is " << int(buf8) << std::endl;
      else std::cout << "Can't get network status" << std::endl;
    if (modem.GetSignalQuality(buf8))
      std::cout << "Signal quality is " << int(buf8) << std::endl;
      else std::cout << "Can't get signal quality" << std::endl;
    Iridium::Modem::SbdStatus status;
    if (modem.SbdGetStatus(status))
      std::cout << "SBD status:" << std::endl
                << "  MO flag is " << status.mo_flag << std::endl
                << "  MOMSN = " << status.momsn << std::endl
                << "  MT flag is " << status.mt_flag << std::endl
                << "  MTMSN = " << status.mtmsn << std::endl
                << "  RA flag is " << status.ra_flag << std::endl
                << "  Waiting messages " << int(status.mt_queue) << std::endl;
      else std::cout << "Can't get SBD status" << std::endl;
    if (modem.SbdClearBuffers(Iridium::Modem::eClearMObuffer))
      std::cout << "MO message buffer successfuly cleared" << std::endl;
      else std::cout << "MO message buffer clear error" << std::endl;
    if (modem.SbdClearBuffers(Iridium::Modem::eClearMTbuffer))
      std::cout << "MT message buffer successfuly cleared" << std::endl;
      else std::cout << "MT message buffer clear error" << std::endl;
    if (modem.SbdClearBuffers(Iridium::Modem::eClearBoth))
      std::cout << "Both message buffers successfuly cleared" << std::endl;
      else std::cout << "Message buffers clear error" << std::endl;
    std::cout << "Detach from SBD return error " << int(buf8) << std::endl;
    std::cout << (modem.ClearMomsn() ?
                  "The MOMSN was cleared successfully." :
                  "An error occurred while clearing the MOMSN.") << std::endl;
    {
      std::string buffer;
      if (modem.GetImei(buffer))
        std::cout << "IMEI: " << buffer << std::endl;
        else std::cout << "Can't get IMEI" << std::endl;
    }
    try
    {
      std::vector<char> payload;
      modem.ReadMessage(payload);
      std::cout << "Message payload size is " << payload.size() << std::endl;
    }
    catch (std::runtime_error& e)
    {
      std::cout << "ReadMessage() error: " << e.what() << std::endl;
    }
    try
    {
      Iridium::Modem::SbdSessionStatus sessionStatus = modem.DoSbdSession(false);
      std::cout << "SBD session status:" << std::endl
                << "  MO status = " << int(sessionStatus.mo_status) << std::endl
                << "  MOMSN = " << sessionStatus.momsn << std::endl
                << "  MT status = " << int(sessionStatus.mt_status) << std::endl
                << "  MTMSN = " << sessionStatus.mtmsn << std::endl
                << "  MT message length = " << sessionStatus.mt_message_len << std::endl
                << "  Waiting messages " << int(sessionStatus.mt_queue) << std::endl;
    }
    catch (std::runtime_error& e)
    {
      std::cout << "DoSbdSession() error: " << e.what() << std::endl;
    }
    modem.SbdDetach(buf8);
#endif
    worker(boost::system::error_code());
    io_service.run();
  }
  return EXIT_SUCCESS;
}
