#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include "iridium/SbdReceiver.hpp"

#define UNUSED(x) (void)x;

const short iridiumPort = 32606;

void daemonize(const std::string& pidFilename, const std::string& workDir)
{
  pid_t pid = fork();
  if (pid < 0) exit(EXIT_FAILURE);
  if (pid > 0) exit(EXIT_SUCCESS);
  setsid();
  pid = fork();
  if (pid < 0) exit(EXIT_FAILURE);
  if (pid > 0) exit(EXIT_SUCCESS);
  if (!workDir.empty())
  {
    if (chdir(workDir.c_str()) == -1) exit(EXIT_FAILURE);
  }
  umask(0);
  int fd = open("/dev/null", O_RDWR, 0);
  if (fd != -1)
  {
#if 0
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
#endif
    if (fd > STDERR_FILENO) close(fd);
  }
  fd = open(pidFilename.c_str(), O_CREAT | O_WRONLY, 0644);
  if (fd != -1)
  {
    dprintf(fd, "%10d\n", static_cast<int>(getpid()));
    close(fd);
  }
  else
  {
    exit(EXIT_FAILURE);
  }
}

bool alreadyRunning(const std::string& pidFilename, const std::string& appName)
{
  struct stat sb;
  // если файла нет, проверять нечего
  if (stat(pidFilename.c_str(), &sb) == -1) return false;
  pid_t pid = 0;
  std::ifstream file;
  file.open(pidFilename);
  // Не можем открыть существующий файл на чтение? Определенно,
  // запускаться не стоит.
  if (!file.is_open()) return true;
  file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  try
  {
    file >> pid;
  }
  catch (...)
  {
    // не смогли прочесть PID, стало быть, ничего не запущено
    return false;
  }
  file.close();
  // проверяем, какое приложение запущено с таким PID
  std::ostringstream out;
  out << "/proc/" << pid << "/cmdline";
  try
  {
    file.open(out.str().c_str());
  }
  catch (...)
  {
    // нет такого файла, значит нет такого приложения
    return false;
  }
  std::string cmdline;
  std::getline(file, cmdline);
  file.close();
  return (cmdline.find(appName) != cmdline.npos);
}

int main()
{
  bool shutdown = false;
  boost::asio::io_service io_service;
  Iridium::SbdReceiver receiver(io_service, iridiumPort);
  boost::asio::signal_set stopSignals(io_service, SIGINT, SIGTERM, SIGQUIT);
  stopSignals.async_wait(
  [&](const boost::system::error_code& error, int signal) {
    UNUSED(signal)
    // ignore signal handling cancellation
    if (error == boost::asio::error::operation_aborted) return;
    shutdown = true;
    try
    {
      receiver.stop();
    }
    catch (std::runtime_error& e)
    {
      std::cerr << "Receiver stop error: " << e.what() << std::endl;
    }
  });
  while (!shutdown)
  {
    try
    {
      receiver.start();
    }
    catch (std::runtime_error& e)
    {
      std::cerr << "Can't start receiver: " << e.what() << std::endl;
      return EXIT_FAILURE;
    }
    io_service.run();
  }
  return EXIT_SUCCESS;
}
