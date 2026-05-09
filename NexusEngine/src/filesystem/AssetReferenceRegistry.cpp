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

        std::filesystem::path NormalizeToProjectRelativePath(const std::filesystem::path& path, const std::filesystem::path& projectRoot)
        {
            if (path.empty())
            {
                return {};
            }

            return path.is_absolute()
                ? std::filesystem::relative(path, projectRoot)
                : path;
        }

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
            return projectRoot / ".nexus" / "assetrefs";
        }

        std::filesystem::path GetAssetMetaFilePath(const std::filesystem::path& path)
        {
            if (path.extension() == ".nmeta")
            {
                return path;
            }

            return path.parent_path() / (path.stem().string() + ".nmeta");
        }
    }

    AssetReference AssetReferenceRegistry::CreateAssetReference(const std::filesystem::path& path)
    {
        bool success = true;
        const AssetReference assetReference{ CreateAssetGuid() };

        // Create a .nmeta file next to the asset to store the asset reference guid. This allows for O(1) lookup of the asset reference by path.
        success &= SaveToFile(
            assetReference,
            path.parent_path() / (path.stem().string() + ".nmeta"),
            FileFormat::Json);

        if (!success)
        {
            return {};
        }

        // Store the mapping between the asset reference guid and the asset path in the registry directory.
        const std::filesystem::path relativePath = NormalizeToProjectRelativePath(path, m_projectRoot);
        const std::filesystem::path assetReferenceDirectory = GetAssetReferenceDirectoryPath(m_projectRoot);

        success &= SaveToFile(
            AssetReferenceRecord{ assetReference.GetGuid(), relativePath },
            assetReferenceDirectory / (assetReference.GetGuid() + ".assetref"),
            FileFormat::Json);

        if (!success)
        {
            return {};
        }

        return assetReference;
    }

    std::filesystem::path AssetReferenceRegistry::ResolveAssetReferencePath(const AssetReference& reference)
    {
        if (reference.m_guid.empty())
        {
            return {};
        }

        const std::filesystem::path assetReferenceDirectory = GetAssetReferenceDirectoryPath(m_projectRoot);

        AssetReferenceRecord assetReferenceRecord;
        LoadFromFile(
            assetReferenceRecord,
            assetReferenceDirectory / (reference.m_guid + ".assetref"),
            FileFormat::Json);
        return assetReferenceRecord.m_path;
    }

    AssetReference AssetReferenceRegistry::LookupAssetReferenceByPath(const std::filesystem::path& path)
    {
        // Use the .nmeta file alongside the asset to look up the asset reference guid. This ensures O(1) lookup time and allows references to remain valid even if the asset is moved or renamed outside of the editor.
        // For now .nmeta files just contain the asset reference guid, but in the future they can be extended to include other metadata about the asset as well.
        AssetReference assetReference;
        LoadFromFile(
            assetReference,
            GetAssetMetaFilePath(path),
            FileFormat::Json);
        return assetReference;
    }

    AssetReference AssetReferenceRegistry::GetOrCreateAssetReferenceByPath(const std::filesystem::path& path)
    {
        AssetReference assetReference = LookupAssetReferenceByPath(path);
        if (!assetReference.m_guid.empty())
        {
            const std::filesystem::path resolvedPath = ResolveAssetReferencePath(assetReference);
            if (!resolvedPath.empty())
            {
                return assetReference;
            }
        }

        const std::filesystem::path relativePath = NormalizeToProjectRelativePath(path, m_projectRoot);
        const std::filesystem::path assetReferenceDirectory = GetAssetReferenceDirectoryPath(m_projectRoot);
        std::error_code errorCode;
        for (std::filesystem::directory_iterator iterator(assetReferenceDirectory, errorCode), end;
             !errorCode && iterator != end;
             iterator.increment(errorCode))
        {
            if (!iterator->is_regular_file(errorCode) || iterator->path().extension() != ".assetref")
            {
                continue;
            }

            AssetReferenceRecord record;
            if (!LoadFromFile(record, iterator->path(), FileFormat::Json))
            {
                continue;
            }

            if (NormalizeToProjectRelativePath(record.m_path, m_projectRoot) != relativePath)
            {
                continue;
            }

            assetReference = AssetReference{ record.m_guid };
            SaveToFile(assetReference, GetAssetMetaFilePath(path), FileFormat::Json);
            return assetReference;
        }

        assetReference = CreateAssetReference(path);
        return assetReference;
    }

    void AssetReferenceRegistry::UpdateAssetReferencePath(const AssetReference& reference, const std::filesystem::path& newPath)
    {
        if (reference.m_guid.empty())
        {
            return;
        }
        const std::filesystem::path assetReferenceDirectory = GetAssetReferenceDirectoryPath(m_projectRoot);
        AssetReferenceRecord assetReferenceRecord;
        assetReferenceRecord.m_guid = reference.GetGuid();
        assetReferenceRecord.m_path = NormalizeToProjectRelativePath(newPath, m_projectRoot);
        SaveToFile(
            assetReferenceRecord,
            assetReferenceDirectory / (reference.m_guid + ".assetref"),
            FileFormat::Json);
    }

    void AssetReferenceRegistry::RemoveAssetReferencesForPath(const std::filesystem::path& path)
    {
        std::error_code errorCode;
        if (std::filesystem::is_directory(path, errorCode))
        {
            for (std::filesystem::recursive_directory_iterator iterator(path, errorCode), end;
                 !errorCode && iterator != end;
                 iterator.increment(errorCode))
            {
                if (!iterator->is_regular_file(errorCode) || iterator->path().extension() != ".nmeta")
                {
                    continue;
                }

                RemoveAssetReferencesForPath(iterator->path());
            }

            return;
        }

        const std::filesystem::path metaPath = GetAssetMetaFilePath(path);
        AssetReference assetReference;
        if (!LoadFromFile(assetReference, metaPath, FileFormat::Json) || assetReference.IsEmpty())
        {
            return;
        }

        const std::filesystem::path assetReferenceDirectory = GetAssetReferenceDirectoryPath(m_projectRoot);
        std::filesystem::remove(assetReferenceDirectory / (assetReference.GetGuid() + ".assetref"), errorCode);
        errorCode.clear();
        std::filesystem::remove(metaPath, errorCode);
    }
} // namespace NexusEngine::IO
