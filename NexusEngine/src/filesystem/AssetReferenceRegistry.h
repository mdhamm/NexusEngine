#pragma once

#include "NexusEngineApi.h"

#include "AssetReference.h"

#include <filesystem>

namespace NexusEngine::IO
{
    // Manages the mapping between asset references (guids) and file paths. This allows assets to be referenced in a way that remains valid even if the asset is moved or renamed.
    class NEXUS_ENGINE_API AssetReferenceRegistry
    {
    public:
        AssetReferenceRegistry(std::filesystem::path projectRoot) : m_projectRoot(projectRoot) {}

        /// <summary>
        /// Creates a new asset reference for the supplied asset path.
        /// </summary>
        /// <param name="path">Asset file path.</param>
        /// <returns>The created asset reference, or an empty reference on failure.</returns>
        AssetReference CreateAssetReference(const std::filesystem::path& path);

        /// <summary>
        /// Resolves an asset reference to its project-relative asset path.
        /// </summary>
        /// <param name="reference">Asset reference to resolve.</param>
        /// <returns>The project-relative asset path, or an empty path when not found.</returns>
        std::filesystem::path ResolveAssetReferencePath(const AssetReference& reference);

        /// <summary>
        /// Looks up an existing asset reference for the supplied asset path.
        /// </summary>
        /// <param name="path">Asset file path.</param>
        /// <returns>The existing asset reference, or an empty reference when none exists.</returns>
        AssetReference LookupAssetReferenceByPath(const std::filesystem::path& path);

        /// <summary>
        /// Looks up an asset reference for the supplied asset path and creates one when missing.
        /// </summary>
        /// <param name="path">Asset file path.</param>
        /// <returns>The existing or newly created asset reference.</returns>
        AssetReference GetOrCreateAssetReferenceByPath(const std::filesystem::path& path);

        /// <summary>
        /// Updates the stored path for an existing asset reference.
        /// </summary>
        /// <param name="reference">Asset reference to update.</param>
        /// <param name="newPath">New asset file path.</param>
        void UpdateAssetReferencePath(const AssetReference& reference, const std::filesystem::path& newPath);

        /// <summary>
        /// Removes registry records and companion metadata for an asset path or directory subtree.
        /// </summary>
        /// <param name="path">Asset file path, metadata file path, or directory to remove from the registry.</param>
        void RemoveAssetReferencesForPath(const std::filesystem::path& path);

    private:
        std::filesystem::path m_projectRoot;
    };

} // namespace NexusEngine::IO
