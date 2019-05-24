#include <cstring>
#include <netinet/in.h>
#include "iridium/IEMtPriority.hpp"

using namespace Iridium::SbdDirectIp;

ContentLength IEMtPriority::unpack(const char* data, ContentLength size)
{
  if (size < IEMtPriority::ElementLength + sizeof(m_length)) return 0;
  std::memcpy(&m_length, data, sizeof(m_length));
  m_length = ntohs(m_length);
  // this element have a fixed length
  if (m_length != IEMtPriority::ElementLength) return 0;
  std::memcpy(&m_content.m_priority, data + sizeof(m_length),
              sizeof(m_content.m_priority));
  m_content.m_priority = ntohs(m_content.m_priority);
  return sizeof(m_length) + m_length;
}

void IEMtPriority::packInto(std::vector<char>& raw)
{
  if ((m_content.m_priority > MinPriority) ||
      (m_content.m_priority < MaxPriority))
    m_content.m_priority = MinPriority;
  m_length = IEMtPriority::ElementLength;
  InformationElement::packInto(raw);
  uint16_t buf16 = htons(m_content.m_priority);
  const char* ptr = reinterpret_cast<char*>(&buf16);
  for (size_t i = 0; i < sizeof(buf16); i++) raw.push_back(ptr[i]);
}
