
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

        //! Process a text through the full PBM pipeline: tokenize -> disassemble -> store
        //! @param text The raw text to process
        //! @param docName Human-readable document name
        //! @param centuryCode Century code for PBM addressing (e.g., "AS" for 19th century)
        //! @return The PBM document address (e.g., "vA.AB.AS.AA.AA"), or empty on failure
        virtual AZStd::string ProcessText(
            const AZStd::string& text,
            const AZStd::string& docName,
            const AZStd::string& centuryCode) = 0;

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
