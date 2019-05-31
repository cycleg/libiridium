#pragma once

#include "InformationElement.hpp"

namespace Iridium {

namespace SbdDirectIp {

struct IEMoConfirmationDto
{
  uint8_t m_status; ///< 1 (Success) or 0 (Failure)

  IEMoConfirmationDto(): m_status(0) {}
};

class IEMoConfirmation:
  public InformationElement,
  public IEContent<IEMoConfirmationDto>
{
  public:
    IEMoConfirmation(): InformationElement(InformationElement::eMoConfirmation) {}

    ContentLength unpack(const char* data, ContentLength size) override;
    void packInto(std::vector<char>& raw) override;

  public:
    static const int ElementLength = 1; ///< This information element have a
                                        ///< fixed length.
}; // class IEMoConfirmation

} // namespace Iridium::SbdDirectIp

} // namespace Iridium
