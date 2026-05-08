#pragma once

#include "AssetReference.h"

#include <string>

namespace NexusEngine::IO
{
    AssetReference AssetReferenceFromPath(const std::string& path);
    AssetReference CreateAssetReference(const std::string& path);
    bool NotifyAssetPathChanged(const std::string& oldPath, const std::string& newPath);
    std::string GetAssetReferencePath(const AssetReference& reference, const std::string& projectRootPath = {});
    bool ResolveAssetReferencePath(const AssetReference& reference, const std::string& projectRootPath, std::string& resolvedPath);

    std::string FindProjectRootPath(const std::string& path);
    std::string GetAssetReferenceDirectoryPath(const std::string& projectRootPath);
    std::string ReadAssetReferenceGuidForPath(const std::string& path);
    std::string EnsureAssetReferenceGuid(const std::string& path);
    bool IsSceneFilePath(const std::string& path);
} // namespace NexusEngine::IO
