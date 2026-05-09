#include "AssetReference.h"

#include "serialization/ISerializer.h"

namespace NexusEngine::IO
{
    void Serialize(const AssetReference& reference, ISerializeWriter& writer)
    {
        writer.Write("guid", reference.m_guid);
        writer.EndObject();
    }

    bool Deserialize(AssetReference& reference, ISerializeReader& reader)
    {
        reader.Read("guid", reference.m_guid);
        reader.EndObject();
        return true;
    }
} // namespace NexusEngine::IO