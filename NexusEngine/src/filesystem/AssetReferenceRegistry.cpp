#include "AssetReferenceRegistry.h"

#include "AssetGuid.h"
#include "serialization/ISerializer.h"
#include "FileIO.h"

namespace NexusEngine::IO
{
    namespace
    {
        struct AssetReferenceRecord
        {
            std::string m_guid;
            std::filesystem::path m_path;
        };

        void Serialize(const AssetReferenceRecord& record, NexusEngine::ISerializeWriter& writer)
        {
            writer.Write("guid", record.m_guid);
            writer.Write("path", record.m_path.string());
            writer.EndObject();
        }

        bool Deserialize(AssetReferenceRecord& record, NexusEngine::ISerializeReader& reader)
        {
            reader.Read("guid", record.m_guid);
            std::string pathString;
            reader.Read("path", pathString);
            record.m_path = pathString;
            return true;
        }

        std::filesystem::path GetAssetReferenceDirectoryPath(const std::filesystem::path& projectRoot)
        {
            return projectRoot / ".nexus" / "assetrefs");
        }
    }

    AssetReference CreateAssetReference(const std::filesystem::path& projectRoot, const std::filesystem::path& path)
    {
        const std::filesystem::path relativePath = std::filesystem::relative(path, projectRoot);
        const std::filesystem::path assetReferenceDirectory = GetAssetReferenceDirectoryPath(projectRoot);

        SaveToFile(
            AssetReferenceRecord{ CreateAssetGuid(), relativePath },
            assetReferenceDirectory / (CreateAssetGuid() + ".json"),
            FileFormat::Json);
    }

    std::filesystem::path ResolveAssetReferencePath(const AssetReference& reference, const std::filesystem::path& projectRoot)
    {
        if (reference.m_guid.empty())
        {
            return {};
        }

        const std::filesystem::path assetReferenceDirectory = GetAssetReferenceDirectoryPath(projectRoot);

        AssetReferenceRecord assetReferenceRecord;
        LoadFromFile(
            assetReferenceRecord,
            assetReferenceDirectory / (reference.m_guid + ".json"),
            FileFormat::Json);
        return assetReferenceRecord.m_path;
    }
} // namespace NexusEngine::IO
