#pragma once

#include <vector>
#include "InformationElement.hpp"

namespace Iridium {

namespace SbdDirectIp {

struct IEMoPayloadDto
{
  std::vector<char> m_payload;
};

class IEMoPayload:
  public InformationElement,
  public IEContent<IEMoPayloadDto>
{
  public:
    static const int MaxPayloadLength = 1960; ///< Max. MO payload in SBDv1.

    IEMoPayload(): InformationElement(InformationElement::eMoPayload) {}

    ContentLength unpack(const char* data, ContentLength size) override;
    void packInto(std::vector<char>& raw) override;
}; // class IEMoPayload

} // namespace Iridium::SbdDirectIp

} // namespace Iridium
