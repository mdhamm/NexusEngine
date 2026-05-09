#pragma once

#include <reflection/EntityReflection.h>
#include <unordered_map>

namespace NexusEditor
{
    std::unordered_map<NexusEngine::AssetType, std::string> GetAssetTypeToExtensionMap();
}