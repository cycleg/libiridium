#pragma once

#include <string>
#include <stdint.h>
#include "IEMtHeader.hpp"
#include "IEMtPriority.hpp"
#include "Message.hpp"

namespace Iridium {

namespace SbdDirectIp {

class Codec
{
  public:
    enum EMessageCategory: uint8_t
    {
      eMoMessage, ///< Mobile originated message.
      eMtMessage, ///< Mobile terminated message.
      eMtConfirmMessage, ///< MT message confirmation message.
      eUnknownMessage ///< Unknown or bad format message.
    };

    static std::string categoryStr(EMessageCategory c);

    ///
    /// Mobile terminated message factory.
    ///
    /// @param [out] out New MT message.
    /// @param [in] msgId Unique client message ID, see 7.2.2.1.2.
    /// @param [in] imei IMEI.
    /// @param [in] payload Outgoing data buffer.
    /// @param [in] size Outgoing data size.
    /// @param [in] flags Message disposition flags, see 7.2.3.
    /// @param [in] priority Message priority, see 7.2.4.
    /// @throw std::runtime_exception
    ///
    static void factory(MtMessage& out, uint32_t msgId,
                        const std::string& imei, const char* payload,
                        size_t size, MtMessageFlags flags = MtMessageFlags(),
                        uint16_t priority = IEMtPriority::MinPriority);
    ///
    /// Check message category.
    ///
    /// @param [in] payload Incoming data buffer.
    /// @param [in] size Incoming data size.
    /// @return Message category.
    ///
    static EMessageCategory messageCategory(const char* payload,
                                            size_t size);
    ///
    /// Parse mobile originated message.
    ///
    /// @param [in] payload Incoming data buffer.
    /// @param [in] size Incoming data size.
    /// @param [out] out New MO message.
    /// @throw std::runtime_exception
    ///
    static void parse(const char* payload, size_t size, MoMessage& out);
    ///
    /// Parse mobile terminated message confirmation message.
    ///
    /// @param [in] payload Incoming data buffer.
    /// @param [in] size Incoming data size.
    /// @param [out] out New MT message confirmation message.
    /// @throw std::runtime_exception
    ///
    static void parse(const char* payload, size_t size, MtConfirmMessage& out);

  private:
    static std::shared_ptr<InformationElement> IEFactory(uint8_t id);
}; // class Codec

} // namespace Iridium::SbdDirectIp

} // namespace Iridium
