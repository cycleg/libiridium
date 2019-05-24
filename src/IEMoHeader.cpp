#include <cstring>
#include <netinet/in.h>
#include "iridium/IEMoHeader.hpp"

using namespace Iridium::SbdDirectIp;

ContentLength IEMoHeader::unpack(const char* data, ContentLength size)
{
  if (size < IEMoHeader::ElementLength + sizeof(m_length)) return 0;
  std::memcpy(&m_length, data, sizeof(m_length));
  m_length = ntohs(m_length);
  // this element have a fixed length
  if (m_length != IEMoHeader::ElementLength) return 0;
  ContentLength offset = sizeof(m_length);
  std::memcpy(&m_content.m_cdrRef, data + offset, sizeof(m_content.m_cdrRef));
  m_content.m_cdrRef = ntohl(m_content.m_cdrRef);
  offset += sizeof(m_content.m_cdrRef);
  std::memset(m_content.m_imei.value, 0, sizeof(m_content.m_imei));
  std::memcpy(m_content.m_imei.value, data + offset, sizeof(m_content.m_imei) - 1);
  offset += sizeof(m_content.m_imei) - 1;
  m_content.m_sessionStatus = static_cast<ESessionStatus>(data[offset]);
  offset++;
  std::memcpy(&m_content.m_momsn, data + offset, sizeof(m_content.m_momsn));
  m_content.m_momsn = ntohs(m_content.m_momsn);
  offset += sizeof(m_content.m_momsn);
  std::memcpy(&m_content.m_mtmsn, data + offset, sizeof(m_content.m_mtmsn));
  m_content.m_mtmsn = ntohs(m_content.m_mtmsn);
  offset += sizeof(m_content.m_mtmsn);
  std::memcpy(&m_content.m_sessionTime, data + offset,
              sizeof(m_content.m_sessionTime));
  m_content.m_sessionTime = ntohl(m_content.m_sessionTime);
  offset += sizeof(m_content.m_sessionTime);
  return offset;
}

void IEMoHeader::packInto(std::vector<char>& raw)
{
  m_length = IEMoHeader::ElementLength;
  InformationElement::packInto(raw);
  uint32_t buf32 = htonl(m_content.m_cdrRef);
  const char* ptr = reinterpret_cast<char*>(&buf32);
  for (size_t i = 0; i < sizeof(buf32); i++) raw.push_back(ptr[i]);
  ptr = m_content.m_imei.value;
  for (size_t i = 0; i < sizeof(m_content.m_imei) - 1; i++) raw.push_back(ptr[i]);
  raw.push_back(static_cast<char>(m_content.m_sessionStatus));
  uint16_t buf16 = htons(m_content.m_momsn);
  ptr = reinterpret_cast<char*>(&buf16);
  for (size_t i = 0; i < sizeof(buf16); i++) raw.push_back(ptr[i]);
  buf16 = htons(m_content.m_mtmsn);
  ptr = reinterpret_cast<char*>(&buf16);
  for (size_t i = 0; i < sizeof(buf16); i++) raw.push_back(ptr[i]);
  buf32 = htonl(m_content.m_sessionTime);
  ptr = reinterpret_cast<char*>(&buf32);
  for (size_t i = 0; i < sizeof(buf32); i++) raw.push_back(ptr[i]);
}
