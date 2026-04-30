#include "ComponentReflection.h"

#include <algorithm>

namespace NexusEngine
{
    ComponentReflectionRegistry& ComponentReflectionRegistry::Instance()
    {
        static ComponentReflectionRegistry s_registry;
        return s_registry;
    }

    void ComponentReflectionRegistry::RegisterDescriptor(ComponentDescriptor descriptor)
    {
        auto it = std::find_if(
            m_descriptors.begin(),
            m_descriptors.end(),
            [&](const ComponentDescriptor& existing)
            {
                return existing.m_name == descriptor.m_name;
            });

        if (it != m_descriptors.end())
        {
            *it = std::move(descriptor);
            return;
        }

        m_descriptors.push_back(std::move(descriptor));
    }

    const std::vector<ComponentDescriptor>& ComponentReflectionRegistry::GetDescriptors() const
    {
        return m_descriptors;
    }
} // namespace NexusEngine
