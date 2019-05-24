#include <cstring>
#include <netinet/in.h>
#include "iridium/IEMtConfirmationMsg.hpp"

using namespace Iridium::SbdDirectIp;

ContentLength IEMtConfirmationMsg::unpack(const char* data, ContentLength size)
{
  if (size < IEMtConfirmationMsg::ElementLength + sizeof(m_length)) return 0;
  std::memcpy(&m_length, data, sizeof(m_length));
  m_length = ntohs(m_length);
  // this element have a fixed length
  if (m_length != IEMtConfirmationMsg::ElementLength) return 0;
  ContentLength offset = sizeof(m_length);
  std::memcpy(&m_content.m_uniqueClientMsgId, data + offset,
              sizeof(m_content.m_uniqueClientMsgId));
  m_content.m_uniqueClientMsgId = ntohl(m_content.m_uniqueClientMsgId);
  offset += sizeof(m_content.m_uniqueClientMsgId);
  std::memset(m_content.m_imei.value, 0, sizeof(m_content.m_imei));
  std::memcpy(m_content.m_imei.value, data + offset,
              sizeof(m_content.m_imei) - 1);
  offset += sizeof(m_content.m_imei) - 1;
  std::memcpy(&m_content.m_autoIdRef, data + offset,
              sizeof(m_content.m_autoIdRef));
  m_content.m_autoIdRef = ntohl(m_content.m_autoIdRef);
  offset += sizeof(m_content.m_autoIdRef);
  std::memcpy(&m_content.m_msgStatus, data + offset,
              sizeof(m_content.m_msgStatus));
  m_content.m_msgStatus = ntohs(m_content.m_msgStatus);
  offset += sizeof(m_content.m_msgStatus);
  return offset;
}

void IEMtConfirmationMsg::packInto(std::vector<char>& raw)
{
  m_length = IEMtConfirmationMsg::ElementLength;
  InformationElement::packInto(raw);
  uint32_t buf32 = htonl(m_content.m_uniqueClientMsgId);
  const char* ptr = reinterpret_cast<char*>(&buf32);
  for (size_t i = 0; i < sizeof(buf32); i++) raw.push_back(ptr[i]);
  ptr = m_content.m_imei.value;
  for (size_t i = 0; i < sizeof(m_content.m_imei) - 1; i++)
    raw.push_back(ptr[i]);
  buf32 = htonl(m_content.m_autoIdRef);
  ptr = reinterpret_cast<char*>(&buf32);
  for (size_t i = 0; i < sizeof(buf32); i++) raw.push_back(ptr[i]);
  uint16_t buf16 = htons(m_content.m_msgStatus);
  ptr = reinterpret_cast<char*>(&buf16);
  for (size_t i = 0; i < sizeof(buf16); i++) raw.push_back(ptr[i]);
}
