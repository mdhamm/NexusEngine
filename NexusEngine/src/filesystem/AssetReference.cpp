#include "AssetReference.h"

#include "serialization/ISerializer.h"

namespace NexusEngine::IO
{
    void Serialize(const AssetReference& reference, ISerializeWriter& writer)
    {
        writer.Write("guid", reference.m_guid);
    }

    bool Deserialize(AssetReference& reference, ISerializeReader& reader)
    {
        reader.Read("guid", reference.m_guid);
        return true;
    }
} // namespace NexusEngine::IO