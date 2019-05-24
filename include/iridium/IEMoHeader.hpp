#pragma once

#include "InformationElement.hpp"

namespace Iridium {

namespace SbdDirectIp {

struct IEMoHeaderDto
{
  uint32_t m_cdrRef;
  IMEI m_imei;
  uint8_t m_sessionStatus;
  uint16_t m_momsn, ///< The mobile-originated message sequence number.
           m_mtmsn; ///< The mobile-terminated message sequence number.
  uint32_t m_sessionTime; ///< UTC timestamp of the IMEI session between
                          ///< the IMEI and the Iridium Gateway.

  IEMoHeaderDto():
    m_cdrRef(0),
    m_sessionStatus(0), // ESessionStatus::eSuccess
    m_momsn(0), m_mtmsn(0), m_sessionTime(0)
  { m_imei = {0}; }
};

class IEMoHeader:
  public InformationElement,
  public IEContent<IEMoHeaderDto>
{
  public:
    static const int ElementLength = 28; ///< This information element have a
                                         ///< fixed length.

    ///
    /// SBD Session Status Values.
    ///
    enum ESessionStatus: uint8_t
    {
      eSuccess = 0, ///< The SBD session completed successfully.
      eMtMessageTooLarge = 1, ///< The MO message transfer, if any, was
                              ///< successful. The MT message queued at the
                              ///< Iridium Gateway is too large to be
                              ///< transferred within a single SBD session.
      eLowQualityLocation = 2, ///< The MO message transfer, if any, was
                               ///< successful. The reported location was
                               ///< determined to be of unacceptable quality.
                               ///< This value is only applicable to IMEIs
                               ///< using SBD protocol revision 1.
      eTimeout = 10, ///< The SBD session timed out before session completion.
      eMoMessageTooLarge = 12, ///< The MO message being transferred by the
                               ///< IMEI is too large to be transferred within
                               ///< a single SBD session.
      eLinkLoss = 13, ///< An RF link loss occurred during the SBD session.
      eProtocolAnomaly = 14, ///< An IMEI protocol anomaly occurred during SBD
                             ///< session.
      eBannedImei = 15 ///< The IMEI is prohibited from accessing the Iridium
                       ///< Gateway.
    };

    IEMoHeader(): InformationElement(InformationElement::eMoHeader) {}

    ContentLength unpack(const char* data, ContentLength size) override;
    void packInto(std::vector<char>& raw) override;
}; // class IEMoHeader

}; // namespace Iridium::SbdDirectIp

}; // namespace Iridium
