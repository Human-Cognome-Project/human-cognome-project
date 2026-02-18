
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include "HCPEngineSystemComponent.h"

#include <HCPEngine/HCPEngineTypeIds.h>

namespace HCPEngine
{
    class HCPEngineModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(HCPEngineModule, HCPEngineModuleTypeId, AZ::Module);
        AZ_CLASS_ALLOCATOR(HCPEngineModule, AZ::SystemAllocator);

        HCPEngineModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                HCPEngineSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<HCPEngineSystemComponent>(),
            };
        }
    };
}// namespace HCPEngine

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), HCPEngine::HCPEngineModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_HCPEngine, HCPEngine::HCPEngineModule)
#endif
