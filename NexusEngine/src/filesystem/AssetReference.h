#pragma once

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

    void Serialize(const AssetReference& reference, ISerializeWriter& writer);
    bool Deserialize(AssetReference& reference, ISerializeReader& reader);

} // namespace NexusEngine::IO
