#include <cstring>
#include <netinet/in.h>
#include "iridium/IEMoConfirmation.hpp"

using namespace Iridium::SbdDirectIp;

ContentLength IEMoConfirmation::unpack(const char* data, ContentLength size)
{
  if (size < IEMoConfirmation::ElementLength + sizeof(m_length)) return 0;
  std::memcpy(&m_length, data, sizeof(m_length));
  m_length = ntohs(m_length);
  // this element have a fixed length
  if (m_length != IEMoConfirmation::ElementLength) return 0;
  ContentLength offset = sizeof(m_length);
  std::memcpy(&m_content.m_status, data + offset, sizeof(m_content.m_status));
  offset += sizeof(m_content.m_status);
  return offset;
}

void IEMoConfirmation::packInto(std::vector<char>& raw)
{
  m_length = IEMoConfirmation::ElementLength;
  InformationElement::packInto(raw);
  raw.push_back(static_cast<char>(m_content.m_status));
}
