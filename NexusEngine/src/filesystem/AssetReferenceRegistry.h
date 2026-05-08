#pragma once

#include "AssetReference.h"

#include <filesystem>

namespace NexusEngine::IO
{
    AssetReference CreateAssetReference(const std::filesystem::path& projectRoot, const std::filesystem::path& path);
    std::filesystem::path ResolveAssetReferencePath(const AssetReference& reference, const std::filesystem::path& projectRoot);
} // namespace NexusEngine::IO
