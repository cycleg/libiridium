#include <cstring>
#include <netinet/in.h>
#include "iridium/IEMoPayload.hpp"

using namespace Iridium::SbdDirectIp;

ContentLength IEMoPayload::unpack(const char* data, ContentLength size)
{
  if (size < sizeof(m_length)) return 0;
  std::memcpy(&m_length, data, sizeof(m_length));
  m_length = ntohs(m_length);
  if ((m_length < 1) || (m_length > IEMoPayload::MaxPayloadLength)) return 0;
  if (size < m_length) return 0;
  ContentLength offset = sizeof(m_length);
  m_content.m_payload.clear();
  m_content.m_payload.resize(m_length);
  std::memcpy(m_content.m_payload.data(), data + offset, m_length);
  offset += m_length;
  return offset;
}

void IEMoPayload::packInto(std::vector<char>& raw)
{
  m_length = m_content.m_payload.size();
  if (m_length > IEMoPayload::MaxPayloadLength)
  {
    // truncate to max. payload length
    m_length = IEMoPayload::MaxPayloadLength;
    m_content.m_payload.resize(m_length);
  }
  InformationElement::packInto(raw);
  for (char c: m_content.m_payload) raw.push_back(c);
}
