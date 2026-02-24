#pragma once

#include "../HCPEngineSystemComponent.h"
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace HCPEngine
{
    //! Editor-side system component. Inherits all runtime functionality and adds
    //! editor panel registration via NotifyRegisterViews().
    class HCPEngineEditorSystemComponent
        : public HCPEngineSystemComponent
        , protected AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = HCPEngineSystemComponent;

    public:
        AZ_COMPONENT_DECL(HCPEngineEditorSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        HCPEngineEditorSystemComponent();
        ~HCPEngineEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AzToolsFramework::EditorEvents::Bus::Handler
        void NotifyRegisterViews() override;
    };

} // namespace HCPEngine
