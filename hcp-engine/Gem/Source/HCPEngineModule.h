#pragma once

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
            m_descriptors.insert(m_descriptors.end(), {
                HCPEngineSystemComponent::CreateDescriptor(),
            });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<HCPEngineSystemComponent>(),
            };
        }
    };
} // namespace HCPEngine
