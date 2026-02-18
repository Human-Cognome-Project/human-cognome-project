
#pragma once

#include <AzCore/Component/Component.h>

#include <HCPEngine/HCPEngineBus.h>
#include "HCPVocabulary.h"
#include "HCPParticlePipeline.h"

namespace HCPEngine
{
    class HCPEngineSystemComponent
        : public AZ::Component
        , protected HCPEngineRequestBus::Handler
    {
    public:
        AZ_COMPONENT_DECL(HCPEngineSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        HCPEngineSystemComponent();
        ~HCPEngineSystemComponent();

    protected:
        ////////////////////////////////////////////////////////////////////////
        // HCPEngineRequestBus interface implementation
        AZStd::string ProcessText(
            const AZStd::string& text,
            const AZStd::string& docName,
            const AZStd::string& centuryCode) override;
        AZStd::string ReassembleFromPBM(const AZStd::string& docId) override;
        bool IsReady() const override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    private:
        HCPVocabulary m_vocabulary;
        HCPParticlePipeline m_particlePipeline;
    };
}
