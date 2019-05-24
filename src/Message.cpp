#include <cstring>
#include <iomanip>
#include <netinet/in.h>
#include "iridium/IEMoHeader.hpp"
#include "iridium/IEMoPayload.hpp"
#include "iridium/IEMoLocationInfo.hpp"
#include "iridium/IEMtHeader.hpp"
#include "iridium/IEMtPayload.hpp"
#include "iridium/IEMtPriority.hpp"
#include "iridium/IEMtConfirmationMsg.hpp"
#include "iridium/Message.hpp"

namespace Iridium {
namespace SbdDirectIp {

const uint16_t MoMessage::MaxMessageSize = InformationElement::HeaderSize * 3 +
                                           IEMoHeader::ElementLength +
                                           IEMoPayload::MaxPayloadLength +
                                           IEMoLocationInfo::ElementLength;

MoMessage::MoMessage():
  m_header(m_elements.end()),
  m_payload(m_elements.end()),
  m_location(m_elements.end())
{
}

std::string MoMessage::imei() const
{
  if (m_header == m_elements.end())
    return std::string();
    else
    {
      IEMoHeader* h = static_cast<IEMoHeader*>(m_header->get());
      return std::string(h->getContent().m_imei.value);
    }
}

std::vector<char> MoMessage::payload() const
{
  if (m_payload == m_elements.end())
    return std::vector<char>();
    else
    {
      IEMoPayload* p = static_cast<IEMoPayload*>(m_payload->get());
      return p->getContent().m_payload;
    }
}

const uint16_t MtMessage::MaxMessageSize = InformationElement::HeaderSize * 3 +
                                           IEMtHeader::ElementLength +
                                           IEMtPayload::MaxPayloadLength +
                                           IEMtPriority::ElementLength;

MtMessage::MtMessage():
  m_header(m_elements.end()),
  m_payload(m_elements.end()),
  m_priority(m_elements.end())
{
}

std::string MtMessage::imei() const
{
  if (m_header == m_elements.end())
    return std::string();
    else
    {
      IEMtHeader* h = static_cast<IEMtHeader*>(m_header->get());
      return std::string(h->getContent().m_imei.value);
    }
}

std::vector<char> MtMessage::payload() const
{
  if (m_payload == m_elements.end())
    return std::vector<char>();
    else
    {
      IEMtPayload* p = static_cast<IEMtPayload*>(m_payload->get());
      return p->getContent().m_payload;
    }
}

std::vector<char> MtMessage::serialize()
{
  MessageHeader header = { SbdProtoNumber, 0 };
  std::vector<char> out;
  out.reserve(MtMessage::MaxMessageSize);
  // allocate header in buffer
  for (size_t i = 0; i < sizeof(MessageHeader); i++) out.push_back(0);
  for (auto& elem: m_elements)
  {
     elem->packInto(out);
     header.m_length += elem->getLength() + InformationElement::HeaderSize;
  }
  // assign header in buffer
  out[0] = static_cast<char>(header.m_proto);
  ContentLength buf = htons(header.m_length);
  std::memcpy(out.data() + sizeof(header.m_proto), &buf, sizeof(buf));
  return out;
}

MtConfirmMessage::MtConfirmMessage(): m_confirmation(m_elements.end())
{
}

std::string MtConfirmMessage::imei() const
{
  if (m_confirmation == m_elements.end())
    return std::string();
    else
    {
      IEMtConfirmationMsg* c = static_cast<IEMtConfirmationMsg*>(m_confirmation->get());
      return std::string(c->getContent().m_imei.value);
    }
}

uint32_t MtConfirmMessage::messageId() const
{
  if (m_confirmation == m_elements.end())
    return 0;
    else
    {
      IEMtConfirmationMsg* c = static_cast<IEMtConfirmationMsg*>(m_confirmation->get());
      return c->getContent().m_uniqueClientMsgId;
    }
}

uint32_t MtConfirmMessage::autoRef() const
{
  if (m_confirmation == m_elements.end())
    return 0;
    else
    {
      IEMtConfirmationMsg* c = static_cast<IEMtConfirmationMsg*>(m_confirmation->get());
      return c->getContent().m_autoIdRef;
    }
}

int16_t MtConfirmMessage::status() const
{
  if (m_confirmation == m_elements.end())
    return -32767;
    else
    {
      IEMtConfirmationMsg* c = static_cast<IEMtConfirmationMsg*>(m_confirmation->get());
      return c->getContent().m_msgStatus;
    }
}

std::ostream& operator<<(std::ostream& out, const MoMessage& m)
{
  out << "Mobile originated message: " << std::endl
      << "  IMEI = " << m.imei() << std::endl
      << "  payload = ";
  std::vector<char> payload(m.payload());
  for (char c: payload) out << c;
  return out;
}

std::ostream& operator<<(std::ostream& out, const MtMessage& m)
{
  out << "Mobile terminated message: " << std::endl
      << "  IMEI = " << m.imei() << std::endl
      << "  payload = ";
  std::vector<char> payload(m.payload());
  for (char c: payload) out << c;
  return out;
}

std::ostream& operator<<(std::ostream& out, const MtConfirmMessage& m)
{
  out << "MT message confirmation: " << std::endl
      << "  IMEI = " << m.imei() << std::endl
      << "  client message ID = " << m.messageId() << std::endl
      << "  auto ID reference = " << m.autoRef() << std::endl
      << "  status = ";
  switch (m.status())
  {
    case 0:
      out << "Successful, no payload in message";
      break;
    case -1:
      out << "Invalid IMEI - too few characters, non-numeric characters";
      break;
    case -2:
      out << "Unknown IMEI - not provisioned on the Iridium Gateway";
      break;
    case -3:
      out << "Payload size exceeded maximum allowed";
      break;
    case -4:
      out << "Payload expected, but none received";
      break;
    case -5:
      out << "MT message queue full (max of 50)";
      break;
    case -6:
      out << "MT resources unavailable";
      break;
    case -7:
      out << "Violation of MT DirectIP protocol error";
      break;
    case -8:
      out << "Ring alerts to the given IMEI are disabled";
      break;
    case -9:
      out << "The given IMEI is not attached (not set to receive ring alerts)";
      break;
    case -10:
      out << "Source IP address rejected by MT filter";
      break;
    case -11:
      out << "MTMSN value is out of range (valid range is 1 - 65535)";
      break;
    default:
      out << m.status();
      break;
  }
  return out;
}

}; // namespace Iridium::SbdDirectIp
}; // namespace Iridium
