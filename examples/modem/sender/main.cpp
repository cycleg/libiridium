#include <chrono>
#include <cstring>
#include <iostream>
#include <vector>
#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include "iridium/Modem.hpp"
#include "LockFile.hpp"

#define UNUSED(x) (void)x;

const char* modemDevice = "/dev/ttyS0";
// таймаут ожидания сброса lock-файла, миллисекунды
const unsigned int LockFileTimeout = 500;

int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " <message>" << std::endl;
    return EXIT_FAILURE;
  }
  bool shutdown = false;
  boost::asio::io_service io_service;
  Iridium::Modem modem(io_service, modemDevice);
  boost::asio::signal_set stopSignals(io_service, SIGINT, SIGTERM, SIGQUIT);
  stopSignals.async_wait(
  [&](const boost::system::error_code& error, int signal) {
    UNUSED(signal)
    // ignore signal handling cancellation
    if (error == boost::asio::error::operation_aborted) return;
    shutdown = true;
    modem.Close();
  });
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
    [&io_service](const boost::system::error_code& error) {
      if (error == boost::asio::error::operation_aborted) return;
      io_service.stop();
    });
    io_service.run();
    io_service.reset();
  }
  if (shutdown) return EXIT_FAILURE;
  std::cout << std::endl << "Device " << modemDevice << " acquired."
            << std::endl;
  lockfile.create();
  try
  {
    modem.Open();
  }
  catch (std::runtime_error& e)
  {
    std::cerr << "Can't open modem: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  uint8_t netstatus = 0;
  if (!modem.NetGetStatus(netstatus))
  {
    std::cerr << "Can't get network status." << std::endl;
    return EXIT_FAILURE;
  }
  // registered in home network or roaming
  if ((netstatus != 1) && (netstatus != 5))
  {
    std::cerr << "ISU not ready (" << int(netstatus) << ")." << std::endl;
    return EXIT_FAILURE;
  }
  modem.GetSignalQuality(netstatus);
  if (!netstatus)
  {
    std::cerr << "Signal quality too low." << std::endl;
    return EXIT_FAILURE;
  }
  if (!modem.SbdClearBuffers(Iridium::Modem::eClearMObuffer))
  {
    std::cerr << "Can't clear MO message buffer." << std::endl;
    return EXIT_FAILURE;
  }
  std::vector<char> payload(argv[1], argv[1] + std::strlen(argv[1]));
  try
  {
    uint8_t res = modem.WriteMessage(payload);
    if (res)
    {
      std::cerr << "Store message error: ";
      if (res == 1) std::cerr << "write timeout";
      if (res == 2) std::cerr << "bad CRC";
      // must be exception
      if (res == 3) std::cerr << "size is not correct";
      if (res > 3) std::cerr << int(res);
      std::cerr << std::endl;
      return EXIT_FAILURE;
    }
  }
  catch (std::runtime_error& e)
  {
    std::cerr << "Store message error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  Iridium::Modem::SbdSessionStatus sessionStatus;
  std::cout << "SBD session... " << std::flush;
  try
  {
    sessionStatus = modem.DoSbdSession(false);
    std::cout << "status:" << std::endl
              << "  MO status = " << int(sessionStatus.mo_status) << std::endl
              << "  MOMSN = " << sessionStatus.momsn << std::endl
              << "  MT status = " << int(sessionStatus.mt_status) << std::endl
              << "  MTMSN = " << sessionStatus.mtmsn << std::endl
              << "  MT message length = " << sessionStatus.mt_message_len << std::endl
              << "  Waiting messages " << int(sessionStatus.mt_queue) << std::endl;
    if (sessionStatus.mo_status < 5)
      std::cout << "Message transferred successfully." << std::endl;
      else std::cout << "Message transfer FAILURE." << std::endl;
  }
  catch (std::runtime_error& e)
  {
    std::cout << " error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return (sessionStatus.mo_status < 5) ? EXIT_SUCCESS : EXIT_FAILURE;
}
