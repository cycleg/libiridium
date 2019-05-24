#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include "iridium/Codec.hpp"
#include "iridium/Modem.hpp"
#include "LockFile.hpp"

#define UNUSED(x) (void)x;

const char* modemDevice = "/dev/ttyS0";
// таймаут ожидания сброса lock-файла, миллисекунды
const unsigned int LockFileTimeout = 500;
// проверка входящего сообщения раз в секунду
const unsigned int loopTimeout = 1;

boost::asio::io_service io_service;
Iridium::Modem modem(io_service, modemDevice);
boost::asio::steady_timer timer(io_service);

void loop(const boost::system::error_code& error)
{
  if (error == boost::asio::error::operation_aborted) return;
  if (!modem.SbdClearBuffers(Iridium::Modem::eClearMObuffer))
  {
    std::cerr << "Can't clear MO message buffer." << std::endl;
    timer.expires_from_now(std::chrono::seconds(loopTimeout));
    timer.async_wait(&loop);
    return;
  }
  try
  {
    Iridium::Modem::SbdSessionStatus sessionStatus = modem.DoSbdSession(false);
    if (sessionStatus.mt_status == 1)
    {
      std::vector<char> payload;
      modem.ReadMessage(payload);
      std::clog << "Message received (" << payload.size() << " bytes): ";
      for (char c: payload)
        if (c < ' ')
          std::clog << " 0x" << std::hex << int(c) << std::dec << " ";
          else std::clog << c;
      std::clog << std::endl;
    }
    if (sessionStatus.mt_status == 2)
    {
      std::cerr << "An error occurred while attempting to perform a mailbox check or receive a message from the GSS."
                << std::endl;
    }
  }
  catch (std::runtime_error& e)
  {
    std::cerr << "Can't get SBD status: " << e.what() << std::endl;
  }
  timer.expires_from_now(std::chrono::seconds(loopTimeout));
  timer.async_wait(&loop);
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
    timer.cancel();
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
      timer.expires_from_now(std::chrono::milliseconds(LockFileTimeout));
      timer.async_wait(
      [](const boost::system::error_code& error) {
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
    loop(boost::system::error_code());
    io_service.run();
  }
  return EXIT_SUCCESS;
}
