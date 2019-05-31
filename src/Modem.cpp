#include <atomic>
#include <condition_variable>
#include <stdexcept>
#include <sstream>
#include <thread>
#include <utility>
#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/cast.hpp> 
#include "iridium/IEMoPayload.hpp" // max payload size
#include "iridium/Modem.hpp"

namespace {

bool isBigEndian()
{
  union {
    uint32_t i;
    char c[4];
  } bint = { 0x01020304 };
  return bint.c[0] == 1; 
}

#if 0
void logBuf(boost::asio::streambuf& buf)
{
  std::istream is(&buf);
  char c;
  std::clog << "VV ";
  while (is.get(c))
  {
    std::clog << c;
    if (c == '\n') std::clog << "VV ";
  }
  std::clog << std::endl;
  is.seekg(0, is.beg);
}
#endif

}

using namespace Iridium;

Modem::Modem(boost::asio::io_service& service, const std::string& device):
  m_service(service),
  m_ioService(),
  m_io(m_ioService),
  m_timer(m_ioService),
  m_deviceName(device)
{
}

Modem::~Modem()
{
  Close();
}

void Modem::Open()
{
  if (isOpen()) return;
  boost::system::error_code ec;
  m_io.open(m_deviceName, ec);
  if (ec) throw std::runtime_error(ec.message());
  m_io.set_option(boost::asio::serial_port_base::baud_rate(19200));
  m_io.set_option(boost::asio::serial_port_base::character_size(8));
  m_io.set_option(boost::asio::serial_port_base::parity(
    boost::asio::serial_port_base::parity::none
  ));
  m_io.set_option(boost::asio::serial_port_base::stop_bits(
    boost::asio::serial_port_base::stop_bits::one
  ));
  // Responses are sent to the DTE in verbose mode.
  boost::asio::write(m_io, boost::asio::buffer("ATQ0V1\r", 9), ec);
  if (ec)
  {
    m_io.close();
    throw std::runtime_error(ec.message());
  }
  boost::asio::streambuf buf;
  read_until_timeout(buf, "OK\r\n", ec, 5);
  if (ec)
  {
    m_io.close();
    throw std::runtime_error(ec.message());
  }
  auto work = std::make_shared<boost::asio::io_service::work>(m_service);
  m_sentinel.swap(work);
}

void Modem::Close()
{
  m_timer.cancel();
  // waiting when read_until_timeout() complete
  std::unique_lock<std::mutex> lock(m_mutex);
  m_sentinel.reset();
  if (isOpen()) m_io.close();
}

bool Modem::NetGetStatus(uint8_t& status)
{
  status = 4; // "unknown"
  if (!isOpen()) return false;
  boost::system::error_code ec;
  boost::asio::write(m_io, boost::asio::buffer("AT+CREG?\r", 9), ec);
  if (ec) return false;
  // return:
  // +CREG:002,004
  // 
  // OK
  boost::asio::streambuf buf;
  read_until_timeout(buf, "OK\r\n", ec, 5);
  if (ec) return false;
  std::istream is(&buf);
  std::string line;
  std::string::size_type pos2, pos = std::string::npos;
  std::getline(is, line);
  std::getline(is, line);
  pos = line.find("+CREG:");
  if (pos == std::string::npos) return false;
  pos = line.find_first_of(',', pos + 6);
  if (pos == std::string::npos) return false;
  pos++;
  pos2 = line.find_first_of('\r', pos);
  try
  {
    status =  boost::numeric_cast<uint8_t>(
      boost::lexical_cast<int>(line.data() + pos, pos2 - pos)
    );
  }
  catch (boost::bad_lexical_cast& e)
  {
    return false;
  }
  return true;
}

bool Modem::GetSignalQuality(uint8_t& quality, bool lastKnown)
{
  quality = 0;
  if (!isOpen()) return false;
  boost::system::error_code ec;
  boost::asio::write(
    m_io,
    lastKnown ? boost::asio::buffer("AT+CSQF?\r", 9) : boost::asio::buffer("AT+CSQ?\r", 8),
    ec
  );
  if (ec) return false;
  // return:
  // +CSQ:4
  // 
  // 000
  // 
  // OK
  boost::asio::streambuf buf;
  std::string line;
  std::istream is(&buf);
  std::string::size_type pos2, pos = std::string::npos;
  if (lastKnown)
    {
      read_until_timeout(buf, "OK\r\n", ec, 5);
      if (ec) return false;
      std::getline(is, line);
      std::getline(is, line);
    }
    else
    {
      read_until_timeout(buf, "\r\n", ec, 50);
      if (ec) return false;
      std::getline(is, line);
      if (line.find("ERROR") != pos) return false;
      read_until_timeout(buf, "\r\n", ec, 50);
      if (ec) return false;
      std::getline(is, line);
      if (line.find("ERROR") != pos) return false;
      read_until_timeout(buf, "OK\r\n", ec, 50);
      if (ec) return false;
    }
  pos = line.find("+CSQ:");
  if (pos == std::string::npos) return false;
  pos += 5;
  pos2 = line.find_first_of('\r', pos);
  try
  {
    quality = boost::numeric_cast<uint8_t>(
      boost::lexical_cast<int>(line.data() + pos, pos2 - pos)
    );
  }
  catch (boost::bad_lexical_cast& e)
  {
    return false;
  }
  return true;
}

bool Modem::SbdDetach(uint8_t& error)
{
  error = 36; // reserved, but indicate arror
  if (!isOpen()) return false;
  boost::system::error_code ec;
  boost::asio::write(m_io, boost::asio::buffer("AT+SBDDET\r", 10), ec);
  if (ec) return false;
  // return:
  // +SBDDET:1,18
  // 
  // OK
  boost::asio::streambuf buf;
  read_until_timeout(buf, "OK\r\n", ec, 5);
  if (ec) return false;
  std::istream is(&buf);
  std::string line;
  std::string::size_type pos2, pos = std::string::npos;
  std::getline(is, line);
  std::getline(is, line);
  pos = line.find("+SBDDET:");
  if (pos == std::string::npos) return false;
  pos += 8;
  pos = line.find_first_of(',', pos);
  if (pos == std::string::npos) return false;
  pos++;
  pos2 = line.find_first_of('\r', pos);
  try
  {
    error = boost::numeric_cast<uint8_t>(
      boost::lexical_cast<int>(line.data() + pos, pos2 - pos)
    );
  }
  catch (boost::bad_lexical_cast& e)
  {
    return false;
  }
  return true;
}

bool Modem::SbdGetStatus(SbdStatus& status)
{
  if (!isOpen()) return false;
  boost::system::error_code ec;
  boost::asio::write(m_io, boost::asio::buffer("AT+SBDSX\r", 9), ec);
  if (ec) return false;
  // return:
  // +SBDSX: 0, 13, 0, -1, 0, 0
  // 
  // OK
  boost::asio::streambuf buf;
  read_until_timeout(buf, "OK\r\n", ec, 5);
  if (ec) return false;
  std::istream is(&buf);
  std::string line;
  std::string::size_type pos2, pos = std::string::npos;
  std::getline(is, line);
  std::getline(is, line);
  pos = line.find("+SBDSX: ");
  if (pos == std::string::npos) return false;
  pos += 8;
  status.mo_flag = (line[pos] == '1');
  pos += 3;
  pos2 = line.find_first_of(',', pos);
  try
  {
    status.momsn = boost::lexical_cast<uint16_t>(line.data() + pos, pos2 - pos);
  }
  catch (boost::bad_lexical_cast& e)
  {
    return false;
  }
  pos = pos2 + 2;
  status.mt_flag = (line[pos] == '1');
  pos += 3;
  pos2 = line.find_first_of(',', pos);
  try
  {
    status.mtmsn = boost::lexical_cast<int32_t>(line.data() + pos, pos2 - pos);
  }
  catch (boost::bad_lexical_cast& e)
  {
    return false;
  }
  pos = pos2 + 2;
  status.ra_flag = (line[pos] == '1');
  pos += 3;
  pos2 = line.find_first_of('\r', pos);
  try
  {
    status.mt_queue = boost::numeric_cast<uint8_t>(
      boost::lexical_cast<int>(line.data() + pos, pos2 - pos)
    );
  }
  catch (boost::bad_lexical_cast& e)
  {
    return false;
  }
  return true;
}

bool Modem::SbdClearBuffers(EClearMessageBuffers clear)
{
  if (!isOpen()) return false;
  boost::system::error_code ec;
  uint8_t cmd[9] = { 'A', 'T', '+', 'S', 'B', 'D', 'D', '0', '\r' };
  cmd[7] += clear;
  boost::asio::write(m_io, boost::asio::buffer(cmd, sizeof(cmd)), ec);
  if (ec) return false;
  // return:
  // 0
  // 
  // OK
  boost::asio::streambuf buf;
  read_until_timeout(buf, "OK\r\n", ec, 5);
  if (ec) return false;
  std::istream is(&buf);
  std::string line;
  std::getline(is, line);
  std::getline(is, line);
  return (line[0] == '0');
}

bool Modem::ClearMomsn()
{
  if (!isOpen()) return false;
  boost::system::error_code ec;
  boost::asio::write(m_io, boost::asio::buffer("AT+SBDC\r", 8), ec);
  if (ec) return false;
  // return:
  // 0
  // 
  // OK
  boost::asio::streambuf buf;
  read_until_timeout(buf, "OK\r\n", ec, 5);
  if (ec) return false;
  std::istream is(&buf);
  std::string line;
  std::getline(is, line);
  std::getline(is, line);
  return (line[0] == '0');
}

uint8_t Modem::WriteMessage(const std::vector<char>& payload)
{
  if (!isOpen())
    throw std::runtime_error("modem not opened");
  if (!payload.size() ||
      (payload.size() > SbdDirectIp::IEMoPayload::MaxPayloadLength))
    throw std::runtime_error("bad payload length");
  boost::system::error_code ec;
  std::ostringstream cmd;
  uint32_t sum = 0;
  for (char c: payload) sum += c;
  union {
    uint16_t i;
    char c[2];
  } crc;
  crc.i = sum & 0xFFFF;
  if (!isBigEndian()) std::swap(crc.c[0], crc.c[1]);
  cmd << "AT+SBDWB=" << payload.size() << "\r";
  boost::asio::write(m_io, boost::asio::buffer(cmd.str().c_str(),
                                               cmd.str().size()), ec);
  if (ec)
    throw std::runtime_error("send command error");
  // return:
  // READY<CR><LF>
  // hex: 52 45 41 44 59 0D 0A
  {
    boost::asio::streambuf buf;
    read_until_timeout(buf, "READY\r\n", ec, 10);
    if (ec)
      throw std::runtime_error("reply receive error");
  }
  boost::asio::write(m_io, boost::asio::buffer(payload), ec);
  if (ec)
    throw std::runtime_error("send payload error");
  boost::asio::write(m_io, boost::asio::buffer(crc.c), ec);
  if (ec)
    throw std::runtime_error("send CRC error");
  // return:
  // 0
  // 
  // OK
  boost::asio::streambuf buf;
  read_until_timeout(buf, "OK\r\n", ec, 5);
  std::istream is(&buf);
  std::string line;
  std::getline(is, line);
  std::getline(is, line);
  std::string::size_type pos = std::string::npos;
  uint8_t res;
  pos = line.find_first_of('\r');
  try
  {
    res = boost::numeric_cast<uint8_t>(boost::lexical_cast<int>(line.data(), pos));
  }
  catch (boost::bad_lexical_cast& e)
  {
    res = 4; // nonexistent return code
  }
  return res;
}

void Modem::ReadMessage(std::vector<char>& payload)
{
  payload.clear();
  if (!isOpen())
    throw std::runtime_error("modem not opened");
  boost::system::error_code ec;
  boost::asio::write(m_io, boost::asio::buffer("AT+SBDRB\r", 9), ec);
  if (ec)
    throw std::runtime_error("send command error");
  {
    // get echo of command or empty line
    boost::asio::streambuf buf;
    read_until_timeout(buf, "\r", ec, 5);
    if (ec)
      throw std::runtime_error("message length receive error");
  }
  union {
    uint16_t i;
    char c[2];
  } smallBuf = { 0 };
  boost::asio::read(m_io, boost::asio::buffer(smallBuf.c), ec);
  if (ec)
    throw std::runtime_error("message length receive error");
  // Iridium ISU AT Command Reference, p. 8.11:
  //
  // * The SBD message is transferred formatted as follows:
  //   {2-byte message length} + {binary SBD message} + {2-byte checksum}
  // ...
  // * If there is no mobile terminated SBD message waiting to be retrieved
  //   from the ISU, the message length and checksum fields will be zero.
  if (!isBigEndian()) std::swap(smallBuf.c[0], smallBuf.c[1]);
  if (smallBuf.i)
  {
    payload.resize(smallBuf.i, 0);
    boost::asio::read(m_io, boost::asio::buffer(payload), ec);
    if (ec)
    {
      payload.clear();
      throw std::runtime_error("message receive error");
    }
  }
  smallBuf.i = 0;
  boost::asio::read(m_io, boost::asio::buffer(smallBuf.c), ec);
  if (ec)
  {
    payload.clear();
    throw std::runtime_error("message CRC receive error");
  }
  if (!isBigEndian()) std::swap(smallBuf.c[0], smallBuf.c[1]);
  if (!payload.empty())
  {
    uint32_t crc = 0;
    for (char c: payload) crc += c;
    if ((crc & 0xFFFF) != smallBuf.i)
    {
      payload.clear();
      throw std::runtime_error("CRC check error");
    }
  }
  {
    boost::asio::streambuf buf;
    read_until_timeout(buf, "OK\r\n", ec, 5);
  }
}

Modem::SbdSessionStatus Modem::DoSbdSession(bool answer)
{
  if (!isOpen())
    throw std::runtime_error("modem not opened");
  boost::system::error_code ec;
  const char* cmd = answer ? "AT+SBDIXA\r" : "AT+SBDIX\r";
  boost::asio::write(m_io, boost::asio::buffer(cmd, (answer ? 10 : 9)), ec);
  if (ec)
    throw std::runtime_error("send command error");
  // return:
  // +SBDIX: 32, 13, 2, 0, 0, 0
  // 
  // OK
  SbdSessionStatus status;
  boost::asio::streambuf buf;
  read_until_timeout(buf, "OK\r\n", ec, 50);
  if (ec) throw std::runtime_error(ec.message());
  std::istream is(&buf);
  std::string line;
  std::string::size_type pos2, pos = std::string::npos;
  std::getline(is, line);
  std::getline(is, line);
  pos = line.find("+SBDIX: ");
  if (pos == std::string::npos)
    throw std::runtime_error("bad session status");
  pos += 8;
  pos2 = line.find_first_of(',', pos);
  try
  {
    status.mo_status = boost::numeric_cast<uint8_t>(
      boost::lexical_cast<int>(line.data() + pos, pos2 - pos)
    );
  }
  catch (boost::bad_lexical_cast& e)
  {
    throw std::runtime_error("bad session status");
  }
  pos = pos2 + 2;
  pos2 = line.find_first_of(',', pos);
  try
  {
    status.momsn = boost::lexical_cast<uint16_t>(line.data() + pos, pos2 - pos);
  }
  catch (boost::bad_lexical_cast& e)
  {
    throw std::runtime_error("bad session status");
  }
  pos = pos2 + 2;
  pos2 = line.find_first_of(',', pos);
  try
  {
    status.mt_status = boost::numeric_cast<uint8_t>(
      boost::lexical_cast<int>(line.data() + pos, pos2 - pos)
    );
  }
  catch (boost::bad_lexical_cast& e)
  {
    throw std::runtime_error("bad session status");
  }
  pos = pos2 + 2;
  pos2 = line.find_first_of(',', pos);
  try
  {
    status.mtmsn = boost::lexical_cast<uint16_t>(line.data() + pos, pos2 - pos);
  }
  catch (boost::bad_lexical_cast& e)
  {
    throw std::runtime_error("bad session status");
  }
  pos = pos2 + 2;
  pos2 = line.find_first_of(',', pos);
  try
  {
    status.mt_message_len = boost::lexical_cast<uint16_t>(line.data() + pos, pos2 - pos);
  }
  catch (boost::bad_lexical_cast& e)
  {
    throw std::runtime_error("bad session status");
  }
  pos = pos2 + 2;
  pos2 = line.find_first_of('\r', pos);
  try
  {
    status.mt_queue = boost::numeric_cast<uint8_t>(
      boost::lexical_cast<int>(line.data() + pos, pos2 - pos)
    );
  }
  catch (boost::bad_lexical_cast& e)
  {
    throw std::runtime_error("bad session status");
  }
  return status;
}

void Modem::read_until_timeout(boost::asio::streambuf& buf,
                               const std::string& delim,
                               boost::system::error_code& ec, uint16_t timeout)
{
  std::unique_lock<std::mutex> lock(m_mutex);
  std::atomic<bool> complete(false);
  std::condition_variable cond;
  m_timer.expires_from_now(std::chrono::seconds(timeout));
  m_timer.async_wait([this](const boost::system::error_code& error) {
    (void)error;
    m_io.cancel();
  });
  boost::asio::async_read_until(m_io, buf, delim,
    [this, &complete, &cond, &ec](const boost::system::error_code& error, std::size_t size) {
      (void)size;
      ec = error;
      m_timer.cancel();
      complete.store(true, std::memory_order_relaxed);
      cond.notify_one();
  });
  std::thread thread([this]() {
    m_ioService.run();
  });
  while (!complete.load(std::memory_order_relaxed))
    cond.wait_for(lock, std::chrono::milliseconds(250));
  thread.join();
  m_ioService.reset();
}
