
#pragma once

#include <HCPEngine/HCPEngineTypeIds.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

namespace HCPEngine
{
    class HCPEngineRequests
    {
    public:
        AZ_RTTI(HCPEngineRequests, HCPEngineRequestsTypeId);
        virtual ~HCPEngineRequests() = default;

        //! Load a PBM and reassemble it back into text
        //! @param docId The PBM document address
        //! @return The reconstructed text, or empty on failure
        virtual AZStd::string ReassembleFromPBM(const AZStd::string& docId) = 0;

        //! Check if the engine subsystems are initialized and ready
        virtual bool IsReady() const = 0;
    };

    class HCPEngineBusTraits
        : public AZ::EBusTraits
    {
    public:
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };

    using HCPEngineRequestBus = AZ::EBus<HCPEngineRequests, HCPEngineBusTraits>;
    using HCPEngineInterface = AZ::Interface<HCPEngineRequests>;

} // namespace HCPEngine
