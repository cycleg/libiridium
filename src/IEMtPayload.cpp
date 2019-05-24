#include <cstring>
#include <netinet/in.h>
#include "iridium/IEMtPayload.hpp"

using namespace Iridium::SbdDirectIp;

ContentLength IEMtPayload::unpack(const char* data, ContentLength size)
{
  if (size < sizeof(m_length)) return 0;
  std::memcpy(&m_length, data, sizeof(m_length));
  m_length = ntohs(m_length);
  if ((m_length < 1) || (m_length > IEMtPayload::MaxPayloadLength)) return 0;
  if (size < m_length) return 0;
  m_content.m_payload.clear();
  m_content.m_payload.resize(m_length);
  std::memcpy(m_content.m_payload.data(), data + sizeof(m_length), m_length);
  return sizeof(m_length) + m_length;
}

void IEMtPayload::packInto(std::vector<char>& raw)
{
  m_length = m_content.m_payload.size();
  if (m_length > IEMtPayload::MaxPayloadLength)
  {
    // truncate to max. payload length
    m_length = IEMtPayload::MaxPayloadLength;
    m_content.m_payload.resize(m_length);
  }
  InformationElement::packInto(raw);
  for (char c: m_content.m_payload) raw.push_back(c);
}
