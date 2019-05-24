#pragma once

#include <vector>
#include "InformationElement.hpp"

namespace Iridium {

namespace SbdDirectIp {

struct IEMtPayloadDto
{
  std::vector<char> m_payload;
};

class IEMtPayload:
  public InformationElement,
  public IEContent<IEMtPayloadDto>
{
  public:
    static const int MaxPayloadLength = 1890; ///< Max. MT payload in SBDv1.

    IEMtPayload(): InformationElement(InformationElement::eMtPayload) {}

    ContentLength unpack(const char* data, ContentLength size) override;
    void packInto(std::vector<char>& raw) override;
}; // class IEMtPayload

}; // namespace Iridium::SbdDirectIp

}; // namespace Iridium
