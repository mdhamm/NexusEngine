#include "EntityReflection.h"

namespace NexusEngine
{
    MetadataRegistry& MetadataRegistry::Instance()
    {
        static MetadataRegistry s_registry;
        return s_registry;
    }

    const ComponentMetadata* MetadataRegistry::Get(TypeId type) const
    {
        auto it = m_components.find(type);
        return it != m_components.end() ? &it->second : nullptr;
    }

    const ComponentMetadata* MetadataRegistry::FindByName(const std::string& name) const
    {
        for (const auto& [type, metadata] : m_components)
        {
            (void)type;
            if (metadata.m_name == name)
            {
                return &metadata;
            }
        }

        return nullptr;
    }

    void MetadataRegistry::Remove(TypeId type)
    {
        m_components.erase(type);
    }
} // namespace NexusEngine
