#include <algorithm>
#include <cctype>
#include <cstring>
#include <stdexcept>
#include <netinet/in.h>
#include "iridium/IEMtPayload.hpp"
#include "iridium/IEMoConfirmation.hpp"
#include "iridium/IEMoHeader.hpp"
#include "iridium/IEMoLocationInfo.hpp"
#include "iridium/IEMoPayload.hpp"
#include "iridium/IEMtConfirmationMsg.hpp"
#include "iridium/Codec.hpp"

using namespace Iridium::SbdDirectIp;

std::string Codec::categoryStr(Codec::EMessageCategory c)
{
  const char* ret = nullptr;
  switch (c)
  {
    case Codec::eMoMessage:
      ret = "mobile originated message";
      break;
    case Codec::eMtMessage:
      ret = "mobile terminated message";
      break;
    case Codec::eMtConfirmMessage:
      ret = "MT message confirmation message";
      break;
    default:
      ret = "unknown or bad format message";
      break;
  }
  return std::string(ret);
}

void Codec::factory(MtMessage& out, uint32_t msgId,
                    const std::string& imei, const char* payload,
                    size_t size, MtMessageFlags flags, uint16_t priority)
{
  if (imei.size() != sizeof(IMEI) - 1)
    throw std::runtime_error("invalid IMEI size");
  for (auto c: imei)
    if (!std::isdigit(c)) throw std::runtime_error("bad IMEI");
  if (!payload || (size < 1))
    throw std::runtime_error("no payload");
  if (size > IEMtPayload::MaxPayloadLength)
    throw std::runtime_error("payload too large");
  std::shared_ptr<IEMtHeader> outHeader(new IEMtHeader);
  outHeader->getContent().m_uniqueClientMsgId = msgId;
  std::memcpy(outHeader->getContent().m_imei.value, imei.c_str(), sizeof(IMEI) - 1);
  outHeader->getContent().m_dispositionFlags = flags;
  std::shared_ptr<IEMtPayload> outPayload(new IEMtPayload);
  for (size_t i = 0; i < size; i++)
    outPayload->getContent().m_payload.push_back(payload[i]);
  std::shared_ptr<IEMtPriority> outPrio(new IEMtPriority);
  outPrio->getContent().m_priority = priority;
  out.m_elements.clear();
  out.m_elements.push_back(outHeader);
  out.m_elements.push_back(outPayload);
  out.m_elements.push_back(outPrio);
  out.m_header = std::find_if(out.m_elements.begin(), out.m_elements.end(),
                              [](const std::shared_ptr<InformationElement>& e) {
    return e->getId() == InformationElement::eMtHeader;
  });
  out.m_payload = std::find_if(out.m_elements.begin(), out.m_elements.end(),
                               [](const std::shared_ptr<InformationElement>& e) {
    return e->getId() == InformationElement::eMtPayload;
  });
  out.m_priority = std::find_if(out.m_elements.begin(), out.m_elements.end(),
                               [](const std::shared_ptr<InformationElement>& e) {
    return e->getId() == InformationElement::eMtMsgPriority;
  });
}

Codec::EMessageCategory Codec::messageCategory(const char* payload, size_t size)
{
  EMessageCategory category = eUnknownMessage;
  const char* currPos = payload;
  while (currPos < payload + size)
  {
    uint8_t id = static_cast<uint8_t>(*currPos);
    currPos++;
    if (InformationElement::IsMoElem(id))
    {
      if (category == eUnknownMessage)
        // first element
        category = eMoMessage;
        else
        {
          // Is element from already found category?
          if (category != eMoMessage) return eUnknownMessage;
        }
    }
    else if (InformationElement::IsMtElem(id))
    {
      if (category == eUnknownMessage)
        {
          // first element
          category = eMtMessage;
          if (InformationElement::IsMtConfirm(id)) category = eMtConfirmMessage;
        }
        else
        {
          // Is element from already found category?
          if (InformationElement::IsMtConfirm(id))
            {
              if (category != eMtConfirmMessage) return eUnknownMessage;
            }
            else
            {
              if (category != eMtMessage) return eUnknownMessage;
            }
        }
    }
    else return eUnknownMessage;
    // move to next element
    ContentLength offset = 0;
    std::memcpy(&offset, currPos, sizeof(offset));
    currPos += sizeof(offset);
    offset = ntohs(offset);
    currPos += offset;
  }
  return category;
}

void Codec::parse(const char* payload, size_t size, MoMessage& out)
{
  const char* currPos = payload;
  out.m_elements.clear();
  out.m_header = out.m_payload = out.m_location = out.m_elements.end();
  while (currPos < payload + size)
  {
    uint8_t id = static_cast<uint8_t>(*currPos);
    currPos++;
    auto element = Codec::IEFactory(id);
    if (!element)
    {
      out.m_elements.clear();
      out.m_header = out.m_payload = out.m_location = out.m_elements.end();
      throw std::runtime_error("unknown information element");
    }
    ContentLength offset = element->unpack(currPos, size - (currPos - payload));
    if (!offset)
    {
      out.m_elements.clear();
      out.m_header = out.m_payload = out.m_location = out.m_elements.end();
      throw std::runtime_error("information element parse error");
    }
    out.m_elements.push_back(element);
    currPos += offset;
  }
  out.m_header = std::find_if(out.m_elements.begin(), out.m_elements.end(),
                              [](const std::shared_ptr<InformationElement>& e) {
    return e->getId() == InformationElement::eMoHeader;
  });
  out.m_payload = std::find_if(out.m_elements.begin(), out.m_elements.end(),
                               [](const std::shared_ptr<InformationElement>& e) {
    return e->getId() == InformationElement::eMoPayload;
  });
  out.m_location = std::find_if(out.m_elements.begin(), out.m_elements.end(),
                                [](const std::shared_ptr<InformationElement>& e) {
    return e->getId() == InformationElement::eMoLocationInfo;
  });
  if (out.m_header == out.m_elements.end())
  {
    out.m_elements.clear();
    throw std::runtime_error("no header found");
  }
  if (out.m_payload == out.m_elements.end())
  {
    out.m_elements.clear();
    throw std::runtime_error("no payload found");
  }
  // MO messages without location is valid
}

void Codec::parse(const char* payload, size_t size, MtConfirmMessage& out)
{
  const char* currPos = payload;
  out.m_elements.clear();
  out.m_confirmation = out.m_elements.end();
  while (currPos < payload + size)
  {
    uint8_t id = static_cast<uint8_t>(*currPos);
    currPos++;
    auto element = Codec::IEFactory(id);
    if (!element)
    {
      out.m_elements.clear();
      out.m_confirmation = out.m_elements.end();
      throw std::runtime_error("unknown information element");
    }
    ContentLength offset = element->unpack(currPos, size - (currPos - payload));
    if (!offset)
    {
      out.m_elements.clear();
      out.m_confirmation = out.m_elements.end();
      throw std::runtime_error("information element parse error");
    }
    out.m_elements.push_back(element);
    currPos += offset;
  }
  out.m_confirmation = std::find_if(out.m_elements.begin(), out.m_elements.end(),
                                [](const std::shared_ptr<InformationElement>& e) {
    return e->getId() == InformationElement::eMtConfirmationMsg;
  });
  if (out.m_confirmation == out.m_elements.end())
  {
    out.m_elements.clear();
    throw std::runtime_error("no MT confirmation found");
  }
}

std::shared_ptr<InformationElement> Codec::IEFactory(uint8_t id)
{
  InformationElement* res = nullptr;
  switch (id)
  {
    case InformationElement::eMoHeader:
      res = new IEMoHeader();
      break;
    case InformationElement::eMoPayload:
      res = new IEMoPayload();
      break;
    case InformationElement::eMoLocationInfo:
      res = new IEMoLocationInfo();
      break;
    case InformationElement::eMoConfirmation:
      res = new IEMoConfirmation();
      break;
    case InformationElement::eMtHeader:
      res = new IEMtHeader();
      break;
    case InformationElement::eMtPayload:
      res = new IEMtPayload();
      break;
    case InformationElement::eMtConfirmationMsg:
      res = new IEMtConfirmationMsg();
      break;
    case InformationElement::eMtMsgPriority:
      res = new IEMtPriority();
      break;
  }
  return std::shared_ptr<InformationElement>(res);
}
