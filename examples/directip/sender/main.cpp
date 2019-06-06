#include <iostream>
#include <string>
#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include "iridium/Codec.hpp"
#include "iridium/IEMtHeader.hpp"
#include "iridium/Message.hpp"
#include "iridium/SbdTransmitter.hpp"

#define UNUSED(x) (void)x;

const char* iridiumHost = "12.47.179.12";
const char* iridiumPort = "10800";
const char* IMEI = "300125061511830";

void OnError(const std::string& error)
{
  std::cerr << "Transmit error: " << error << std::endl;
}

void OnResult(int16_t res)
{
  std::cerr << "Transmit status: " << res << std::endl;
}

int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " <message>" << std::endl;
    return EXIT_FAILURE;
  }
  uint32_t msgCount = 1;
  bool shutdown = false;
  std::string payload(argv[1]);
  boost::asio::io_service io_service;
  Iridium::SbdTransmitter transmitter(io_service, iridiumHost, iridiumPort);
  boost::asio::signal_set stopSignals(io_service, SIGINT, SIGTERM, SIGQUIT);
  stopSignals.async_wait(
  [&](const boost::system::error_code& error, int signal) {
    UNUSED(signal)
    // ignore signal handling cancellation
    if (error == boost::asio::error::operation_aborted) return;
    shutdown = true;
    transmitter.stop();
  });
  std::vector<boost::signals2::connection> transmitterConnections;
  transmitterConnections.push_back(transmitter.OnErrorConnect(&OnError));
  transmitterConnections.push_back(transmitter.OnTransmitResultConnect(&OnResult));
  while (!shutdown)
  {
    transmitter.start();
    Iridium::SbdDirectIp::MtMessage message;
    Iridium::SbdDirectIp::MtMessageFlags flags;
    flags.assignMtmsh = 1;
    Iridium::SbdDirectIp::Codec::factory(message, msgCount, IMEI, payload.data(),
                                         payload.size(), flags);
    msgCount++;
    if (msgCount == 65536) msgCount = 1;
    transmitter.post(message);
    io_service.run();
  }
  for (auto& c: transmitterConnections) c.disconnect();
  return EXIT_SUCCESS;
}
