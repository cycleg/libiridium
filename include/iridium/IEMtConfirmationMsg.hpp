#pragma once

#include "InformationElement.hpp"

namespace Iridium {

namespace SbdDirectIp {

struct IEMtConfirmationMsgDto
{
  uint32_t m_uniqueClientMsgId; ///< ID from client (not MTMSN).
  IMEI m_imei;
  uint32_t m_autoIdRef; ///< A unique reference for identifying the MT
                        ///< payload within the SBD database.
  int16_t m_msgStatus; ///< Order of message in IMEI's queue or error reference.

  IEMtConfirmationMsgDto():
    m_uniqueClientMsgId(0), m_imei{0}, m_autoIdRef(0), m_msgStatus(0)
  {}
};

class IEMtConfirmationMsg:
  public InformationElement,
  public IEContent<IEMtConfirmationMsgDto>
{
  public:
    enum EMsgStatus: int16_t
    {
      eSuccess = 0, ///< Successful, no payload in MT message.
      eInvalidImei = -1, ///< Invalid IMEI -- too few characters, non-numeric
                         ///< characters.
      eUnknownImei = -2, ///< Unknown IMEI -- not provisioned on the Iridium
                         ///< Gateway.
      ePayloadTooLarge= -3, ///< Payload size exceeded maximum allowed.
      eNoPayload = -4, ///< Payload expected, but none received.
      eQueueFull = -5, ///< MT message queue full (max of 50).
      eNoResources = -6, ///< MT resources unavailable.
      eProtocolError = -7, ///< Violation of MT DirectIP protocol error.
      eRingAlertsDisabled = -8, ///< Ring alerts to the given IMEI are disabled.
      eImeiNotAttached = -9, ///< The given IMEI is not attached (not set to
                             ///< receive ring alerts).
      eIpAddressRejected = -10, ///< Source IP address rejected by MT filter.
      eMtmsnOutOfRange = -11, ///< MTMSN value is out of range (valid range is
                              ///< 1-65535).
    };

    IEMtConfirmationMsg(): InformationElement(InformationElement::eMtConfirmationMsg) {}

    ContentLength unpack(const char* data, ContentLength size) override;
    void packInto(std::vector<char>& raw) override;

  private:
    static const int ElementLength = 25; ///< This information element have a
                                         ///< fixed length.
}; // class IEMtConfirmationMsg

} // namespace Iridium::SbdDirectIp

} // namespace Iridium
