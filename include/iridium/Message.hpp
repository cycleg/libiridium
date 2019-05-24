#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <vector>
#include "InformationElement.hpp"

namespace Iridium {

namespace SbdDirectIp {

static const uint8_t SbdProtoNumber = 1;

#pragma pack(push, 1)
///
/// Mobile originated (MO) and mobile terminated (MT) message header.
///
struct MessageHeader
{
  int8_t m_proto; ///< Protocol revision number.
  ContentLength m_length; ///< Overall message length.
};
#pragma pack(pop)

class Codec;

class Message
{
  friend class Codec;

  public:
    virtual ~Message() {}

    virtual std::string imei() const = 0;
    virtual std::vector<char> payload() const = 0;
    virtual std::vector<char> serialize() { return std::vector<char>(); };

  protected:
    typedef std::shared_ptr<InformationElement> Pointer;

    std::vector<Pointer> m_elements;
}; // class Message

class IEMoHeader;
class IEMoPayload;
class IEMoLocationInfo;

class MoMessage: public Message
{
  friend class Codec;

  public:
    static const uint16_t MaxMessageSize; ///< Maximum mobile originated
                                         ///< message size (bytes), sending
                                         ///< over Direct IP.

    MoMessage();

    std::string imei() const override;
    std::vector<char> payload() const override;

  private:
    std::vector<Message::Pointer>::iterator m_header,
                                            m_payload,
                                            m_location;
}; // class MoMessage

class IEMtHeader;
class IEMtPayload;
class IEMtPriority;

class MtMessage: public Message
{
  friend class Codec;

  public:
    static const uint16_t MaxMessageSize; ///< Maximum mobile terminated
                                          ///< message size (bytes), sending
                                          ///< over Direct IP.

    MtMessage();

    std::string imei() const override;
    std::vector<char> payload() const override;
    std::vector<char> serialize() override;

  private:
    std::vector<Message::Pointer>::iterator m_header,
                                            m_payload,
                                            m_priority;
}; // class MoMessage

class IEMtConfirmationMsg;

class MtConfirmMessage
{
  friend class Codec;

  public:
    MtConfirmMessage();

    std::string imei() const;
    uint32_t messageId() const;
    uint32_t autoRef() const;
    int16_t status() const;

  private:
    typedef std::shared_ptr<InformationElement> Pointer;

    std::vector<Pointer> m_elements;
    std::vector<Pointer>::iterator m_confirmation;
}; // class MtConfirmMessage

std::ostream& operator<<(std::ostream& out, const MoMessage& m);
std::ostream& operator<<(std::ostream& out, const MtMessage& m);
std::ostream& operator<<(std::ostream& out, const MtConfirmMessage& m);

}; // namespace Iridium::SbdDirectIp

}; // namespace Iridium
