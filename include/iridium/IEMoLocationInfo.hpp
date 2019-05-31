#pragma once

#include "InformationElement.hpp"

namespace Iridium {

namespace SbdDirectIp {

struct IEMoLocationInfoDto
{
  struct {
    unsigned int reserved: 4; ///< Always 0.
    unsigned int formatCode: 2; ///< Always 0.
    unsigned int NSI: 1; ///< North/South Indicator (0 = North, 1 = South).
    unsigned int EWI: 1; ///< East/West Indicator (0 = East, 1 = West).
  } m_flags;
  uint8_t m_latitude; ///< Range: 0 to 90.
  uint16_t m_latitudeMinutes; ///< Thousandths of a minute, range: 0 to 59999.
  uint8_t m_longitude; ///< Range: 0 to 180.
  uint16_t m_longitudeMinutes; ///< Thousandths of a minute, range: 0 to 59999.
  uint32_t m_cepRadius; ///< Radius in kilometers; range: 1 to 2000.

  IEMoLocationInfoDto():
    m_latitude(0), m_latitudeMinutes(0), m_longitude(0), m_longitudeMinutes(0),
    m_cepRadius(0)
  { m_flags.reserved = m_flags.formatCode = m_flags.NSI = m_flags.EWI = 0; }
};

class IEMoLocationInfo:
  public InformationElement,
  public IEContent<IEMoLocationInfoDto>
{
  public:
    static const int ElementLength = 11; ///< This information element have a
                                         ///< fixed length.

    IEMoLocationInfo(): InformationElement(InformationElement::eMoLocationInfo) {}

    ContentLength unpack(const char* data, ContentLength size) override;
    void packInto(std::vector<char>& raw) override;
}; // class IEMoLocationInfo

} // namespace Iridium::SbdDirectIp

} // namespace Iridium
