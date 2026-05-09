#include "FileExtensionInfo.h"

namespace NexusEditor
{
    std::unordered_map<NexusEngine::AssetType, std::string> GetAssetTypeToExtensionMap()
    {
        return
        {
            { NexusEngine::AssetType::Mesh, ".nmesh" },
            { NexusEngine::AssetType::Material, ".nmat" },
            { NexusEngine::AssetType::Scene, ".nscene" }
        };
    }
}