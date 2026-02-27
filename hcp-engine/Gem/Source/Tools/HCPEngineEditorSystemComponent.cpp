#include "HCPEngineEditorSystemComponent.h"
#include "HCPEngineWidget.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <HCPEngine/HCPEngineTypeIds.h>

namespace HCPEngine
{
    AZ_COMPONENT_IMPL(HCPEngineEditorSystemComponent,
        "HCPEngineEditorSystemComponent",
        HCPEngineEditorSystemComponentTypeId,
        BaseSystemComponent);

    void HCPEngineEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<HCPEngineEditorSystemComponent, HCPEngineSystemComponent, AZ::Component>()
                ->Version(0);
        }
    }

    HCPEngineEditorSystemComponent::HCPEngineEditorSystemComponent() = default;
    HCPEngineEditorSystemComponent::~HCPEngineEditorSystemComponent() = default;

    void HCPEngineEditorSystemComponent::GetProvidedServices(
        AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("HCPEngineEditorService"));
    }

    void HCPEngineEditorSystemComponent::GetIncompatibleServices(
        AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("HCPEngineEditorService"));
    }

    void HCPEngineEditorSystemComponent::GetRequiredServices(
        [[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void HCPEngineEditorSystemComponent::GetDependentServices(
        [[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void HCPEngineEditorSystemComponent::Activate()
    {
        HCPEngineSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void HCPEngineEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        HCPEngineSystemComponent::Deactivate();
    }

    void HCPEngineEditorSystemComponent::NotifyRegisterViews()
    {
        AzToolsFramework::ViewPaneOptions options;
        options.paneRect = QRect(100, 100, 900, 700);
        options.preferedDockingArea = Qt::RightDockWidgetArea;
        options.isDeletable = false;
        options.showInMenu = true;

        AzToolsFramework::RegisterViewPane<HCPEngineWidget>(
            "HCP Asset Manager",
            "HCP Engine",
            options);
    }

} // namespace HCPEngine
