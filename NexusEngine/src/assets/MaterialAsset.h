#pragma once

#include "NexusEngineApi.h"
#include "filesystem/FileIO.h"

#include <string>

namespace NexusEngine
{
    constexpr inline auto MaterialAssetFileExtension = ".nmat";

    struct MaterialAsset
    {
        std::string m_name;
        std::string m_vertexShaderPath;
        std::string m_pixelShaderPath;
        std::string m_cullMode = "None";
        bool m_isTransparent = false;
        bool m_depthTestEnabled = true;
        bool m_depthWriteEnabled = true;
    };

    NEXUS_ENGINE_API void Serialize(const MaterialAsset& materialAsset, ISerializeWriter& writer);
    NEXUS_ENGINE_API bool Deserialize(MaterialAsset& materialAsset, ISerializeReader& reader);

    NEXUS_ENGINE_API bool CreateEmptyMaterialAssetFile(const std::filesystem::path& filePath, std::string_view materialName);
    NEXUS_ENGINE_API bool LoadMaterialAssetFile(const std::filesystem::path& filePath, MaterialAsset& materialAsset);
    NEXUS_ENGINE_API bool SaveMaterialAssetFile(const std::filesystem::path& filePath, const MaterialAsset& materialAsset);
    NEXUS_ENGINE_API bool IsMaterialAssetFilePath(std::string_view filePath);
} // namespace NexusEngine
