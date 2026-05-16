#pragma once

#include "NexusEngineApi.h"

#include <string>

namespace NexusEngine
{
    class ISerializeWriter;
    class ISerializeReader;
}

namespace NexusEngine::IO
{
    // Indirect reference to an asset by its guid. This is used to reference assets in a way that remains valid even if the asset is moved or renamed.
    // See also: AssetReferenceRegistry, which manages the mapping between asset guids and file paths.
    struct AssetReference
    {
        std::string m_guid;

        bool IsEmpty() const
        {
            return m_guid.empty();
        }

        std::string GetGuid() const
        {
            return m_guid;
        }
    };

    NEXUS_ENGINE_API void Serialize(const AssetReference& reference, ISerializeWriter& writer);
    NEXUS_ENGINE_API bool Deserialize(AssetReference& reference, ISerializeReader& reader);

} // namespace NexusEngine::IO
