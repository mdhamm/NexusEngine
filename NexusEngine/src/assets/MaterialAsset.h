#pragma once

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

    void Serialize(const MaterialAsset& materialAsset, ISerializeWriter& writer);
    bool Deserialize(MaterialAsset& materialAsset, ISerializeReader& reader);

    bool CreateEmptyMaterialAssetFile(const std::filesystem::path& filePath, std::string_view materialName);
    bool LoadMaterialAssetFile(const std::filesystem::path& filePath, MaterialAsset& materialAsset);
    bool SaveMaterialAssetFile(const std::filesystem::path& filePath, const MaterialAsset& materialAsset);
    bool IsMaterialAssetFilePath(std::string_view filePath);
} // namespace NexusEngine
