#include <cstring>
#include <netinet/in.h>
#include "iridium/IEMoLocationInfo.hpp"

using namespace Iridium::SbdDirectIp;

ContentLength IEMoLocationInfo::unpack(const char* data, ContentLength size)
{
  if (size < IEMoLocationInfo::ElementLength + sizeof(m_length)) return 0;
  std::memcpy(&m_length, data, sizeof(m_length));
  m_length = ntohs(m_length);
  // this element have a fixed length
  if (m_length != IEMoLocationInfo::ElementLength) return 0;
  ContentLength offset = sizeof(m_length);
  std::memcpy(&m_content.m_flags, data + offset, sizeof(m_content.m_flags));
  offset += sizeof(m_content.m_flags);
  std::memcpy(&m_content.m_latitude, data + offset, sizeof(m_content.m_latitude));
  offset += sizeof(m_content.m_latitude);
  std::memcpy(&m_content.m_latitudeMinutes, data + offset,
              sizeof(m_content.m_latitudeMinutes));
  m_content.m_latitudeMinutes = ntohs(m_content.m_latitudeMinutes); // ???
  offset += sizeof(m_content.m_latitudeMinutes);
  std::memcpy(&m_content.m_longitude, data + offset,
              sizeof(m_content.m_longitude));
  offset += sizeof(m_content.m_longitude);
  std::memcpy(&m_content.m_longitudeMinutes, data + offset,
              sizeof(m_content.m_longitudeMinutes));
  m_content.m_longitudeMinutes = ntohs(m_content.m_longitudeMinutes); // ???
  offset += sizeof(m_content.m_longitudeMinutes);
  std::memcpy(&m_content.m_cepRadius, data + offset,
              sizeof(m_content.m_cepRadius));
  m_content.m_cepRadius = ntohl(m_content.m_cepRadius);
  offset += sizeof(m_content.m_cepRadius);
  return offset;
}

void IEMoLocationInfo::packInto(std::vector<char>& raw)
{
  m_length = IEMoLocationInfo::ElementLength;
  InformationElement::packInto(raw);
  const char* ptr = reinterpret_cast<char*>(&m_content.m_flags);
  for (size_t i = 0; i < sizeof(m_content.m_flags); i++) raw.push_back(ptr[i]);
  ptr = reinterpret_cast<char*>(&m_content.m_latitude);
  for (size_t i = 0; i < sizeof(m_content.m_latitude); i++) raw.push_back(ptr[i]);
  uint16_t buf16 = htons(m_content.m_latitudeMinutes); // ???
  ptr = reinterpret_cast<char*>(&buf16);
  for (size_t i = 0; i < sizeof(buf16); i++) raw.push_back(ptr[i]);
  ptr = reinterpret_cast<char*>(&m_content.m_longitude);
  for (size_t i = 0; i < sizeof(m_content.m_longitude); i++)
     raw.push_back(ptr[i]);
  buf16 = htons(m_content.m_longitudeMinutes); // ???
  ptr = reinterpret_cast<char*>(&buf16);
  for (size_t i = 0; i < sizeof(buf16); i++) raw.push_back(ptr[i]);
  uint32_t buf32 = htons(m_content.m_cepRadius);
  ptr = reinterpret_cast<char*>(&buf32);
  for (size_t i = 0; i < sizeof(buf32); i++) raw.push_back(ptr[i]);
}
