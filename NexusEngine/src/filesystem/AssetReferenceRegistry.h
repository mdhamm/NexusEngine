#pragma once

#include "AssetReference.h"

#include <filesystem>

namespace NexusEngine::IO
{
    // Manages the mapping between asset references (guids) and file paths. This allows assets to be referenced in a way that remains valid even if the asset is moved or renamed.
    class AssetReferenceRegistry
    {
    public:
        AssetReferenceRegistry(std::filesystem::path projectRoot) : m_projectRoot(projectRoot) {}

        AssetReference CreateAssetReference(const std::filesystem::path& path);
        std::filesystem::path ResolveAssetReferencePath(const AssetReference& reference);
        AssetReference LookupAssetReferenceByPath(const std::filesystem::path& path);
        AssetReference GetOrCreateAssetReferenceByPath(const std::filesystem::path& path);
        void UpdateAssetReferencePath(const AssetReference& reference, const std::filesystem::path& newPath);

    private:
        std::filesystem::path m_projectRoot;
    };

} // namespace NexusEngine::IO
