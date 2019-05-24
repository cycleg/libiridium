#include <cstring>
#include <netinet/in.h>
#include "iridium/InformationElement.hpp"

using namespace Iridium::SbdDirectIp;

InformationElement::InformationElement(EInformationElementId id):
  m_id(id),
  m_length(0)
{
}

InformationElement::~InformationElement()
{
}

ContentLength InformationElement::unpack(const char* data, ContentLength size)
{
  (void)data;
  (void)size;
  return 0;
}

void InformationElement::packInto(std::vector<char>& raw)
{
#pragma pack(push, 1)
  struct {
    uint8_t id;
    ContentLength length;
  } header;
#pragma pack(pop)
  header.id = m_id;
  header.length = htons(m_length);
  const char* ptr = reinterpret_cast<char*>(&header);
  for (size_t i = 0; i < sizeof(header); i++) raw.push_back(ptr[i]);
}
