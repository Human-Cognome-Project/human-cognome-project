#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include "HCPEngineEditorSystemComponent.h"
#include <HCPEngine/HCPEngineTypeIds.h>

namespace HCPEngine
{
    class HCPEngineEditorModule : public AZ::Module
    {
    public:
        AZ_RTTI(HCPEngineEditorModule, HCPEngineEditorModuleTypeId, AZ::Module);
        AZ_CLASS_ALLOCATOR(HCPEngineEditorModule, AZ::SystemAllocator);

        HCPEngineEditorModule()
            : AZ::Module()
        {
            m_descriptors.insert(m_descriptors.end(), {
                HCPEngineEditorSystemComponent::CreateDescriptor(),
            });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<HCPEngineEditorSystemComponent>(),
            };
        }
    };

} // namespace HCPEngine

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), HCPEngine::HCPEngineEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_HCPEngine_Editor, HCPEngine::HCPEngineEditorModule)
#endif
