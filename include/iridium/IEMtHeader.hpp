#pragma once

#include "InformationElement.hpp"

namespace Iridium {

namespace SbdDirectIp {

struct MtMessageFlags {
  uint16_t flushMtQueue: 1; ///< Delete all MT payloads in the SSD’s
                                ///< MT queue.
  uint16_t sendRingAlert: 1; ///< Send ring alert with no associated
                                 ///< MT payload (normal ring alert
                                 ///< rules apply).
  uint16_t unused: 1;
  uint16_t updateSsdLocation: 1; ///< Update SSD location with given
                                     ///< lat/lon values.
  uint16_t highPriority: 1; ///< Place the associated MT payload in
                                ///< queue based on priority level.
  uint16_t assignMtmsh: 1; ///< Use the value in the Unique ID field
                               ///< as the MTMSN.
  uint16_t reserved: 10;

  MtMessageFlags():
    flushMtQueue(0), sendRingAlert(0), unused(0), updateSsdLocation(0),
    highPriority(0), assignMtmsh(0), reserved(0)
  {}

   inline uint16_t get() const
   { return flushMtQueue + (sendRingAlert << 1) + (updateSsdLocation << 3) +
            (highPriority << 4) + (assignMtmsh << 5); }
};

struct IEMtHeaderDto
{
  uint32_t m_uniqueClientMsgId; ///< ID from client (not MTMSN).
  IMEI m_imei;
  MtMessageFlags m_dispositionFlags;

  IEMtHeaderDto(): m_uniqueClientMsgId(0), m_imei{0} {}
};

class IEMtHeader:
  public InformationElement,
  public IEContent<IEMtHeaderDto>
{
  public:
    static const int ElementLength = 21; ///< This information element have a
                                         ///< fixed length.

    IEMtHeader(): InformationElement(InformationElement::eMtHeader) {}

    ContentLength unpack(const char* data, ContentLength size) override;
    void packInto(std::vector<char>& raw) override;
}; // class IEMtHeader

} // namespace Iridium::SbdDirectIp

} // namespace Iridium
