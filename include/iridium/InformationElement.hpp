#pragma once

#include <vector>
#include <stdint.h>

namespace Iridium {

namespace SbdDirectIp {

typedef uint16_t ContentLength;
struct IMEI { char value[16]; };

class InformationElement
{
  public:
    ///
    /// Size of information element header in bytes.
    ///
    static const uint16_t HeaderSize = sizeof(uint8_t) + sizeof(ContentLength);

    ///
    /// Message information element identifiers (SBDv1).
    ///
    enum EInformationElementId: uint8_t
    {
      eMoHeader = 0x01,
      eMoPayload = 0x02,
      eMoLocationInfo = 0x03,
      eMoConfirmation = 0x05,
      eMtHeader = 0x41,
      eMtPayload = 0x42,
      eMtConfirmationMsg = 0x44,
      eMtMsgPriority = 0x46
    };

    InformationElement(EInformationElementId id);
    virtual ~InformationElement();

    static inline bool IsMoElem(uint8_t id)
    { return (id >= eMoHeader) && (id <= eMoConfirmation); }
    static inline bool IsMtElem(uint8_t id)
    { return (id >= eMtHeader) && (id <= eMtMsgPriority) && (id != 0x45)  &&
             (id != 0x45); }
    static inline bool IsMtConfirm(uint8_t id)
    { return id == eMtConfirmationMsg; }

    inline EInformationElementId getId() const { return m_id; }
    ///
    /// Get raw content length.
    ///
    /// @return Content length.
    ///
    /// Full element size equal content length add HeaderSize.
    ///
    inline ContentLength getLength() const { return m_length; }

    ///
    /// Parse raw data into element content (deserialze).
    ///
    /// @param [in] data Buffer.
    /// @param [in] size Size of buffer.
    /// @return 0 -- parsing error, otherwise -- number of consumed bytes.
    /// 
    virtual ContentLength unpack(const char* data, ContentLength size);
    ///
    /// Pack element into transfer format.
    ///
    /// @param [in] raw Outgoing data buffer.
    ///
    /// Data append to the end of buffer.
    ///
    virtual void packInto(std::vector<char>& raw);

  protected:
    EInformationElementId m_id; ///< Information element ID.
    ContentLength m_length; ///< information element length (without ID).
}; // class InformationElement

template<typename Content> class IEContent
{
  public:
    virtual ~IEContent() {}

    ///
    /// Get raw content.
    ///
    /// @return Pointer to the buffer.
    ///
    /// This object is an owner of the buffer.
    ///
    inline Content& getContent() { return m_content; }

  protected:
    Content m_content; ///< Information element content, variable.
}; // class IEContent

}; // namespace Iridium::SbdDirectIp

}; // namespace Iridium
