#pragma once

#include "InformationElement.hpp"

namespace Iridium {

namespace SbdDirectIp {

struct IEMtPriorityDto
{
  uint16_t m_priority; ///< Message priority, 1-5 (5 is default and lowest).

  IEMtPriorityDto(): m_priority(5) {}
};

class IEMtPriority:
  public InformationElement,
  public IEContent<IEMtPriorityDto>
{
  public:
    static const int ElementLength = 2; ///< This information element have a
                                        ///< fixed length.
    static const uint16_t MaxPriority = 1;
    static const uint16_t MinPriority = 5;

    IEMtPriority(): InformationElement(InformationElement::eMtMsgPriority) {}

    ContentLength unpack(const char* data, ContentLength size) override;
    void packInto(std::vector<char>& raw) override;
}; // class IEMtPriority

} // namespace Iridium::SbdDirectIp

} // namespace Iridium
