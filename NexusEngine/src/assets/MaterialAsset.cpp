#include "MaterialAsset.h"

#include "serialization/ISerializer.h"

namespace NexusEngine
{
    void Serialize(const MaterialAsset& materialAsset, ISerializeWriter& writer)
    {
        writer.Write("name", materialAsset.m_name);
        writer.Write("vertexShaderPath", materialAsset.m_vertexShaderPath);
        writer.Write("pixelShaderPath", materialAsset.m_pixelShaderPath);
        writer.Write("cullMode", materialAsset.m_cullMode);
        writer.Write("isTransparent", materialAsset.m_isTransparent);
        writer.Write("depthTestEnabled", materialAsset.m_depthTestEnabled);
        writer.Write("depthWriteEnabled", materialAsset.m_depthWriteEnabled);
        writer.EndObject();
    }

    bool Deserialize(MaterialAsset& materialAsset, ISerializeReader& reader)
    {
        materialAsset = {};
        reader.Read("name", materialAsset.m_name);
        reader.Read("vertexShaderPath", materialAsset.m_vertexShaderPath);
        reader.Read("pixelShaderPath", materialAsset.m_pixelShaderPath);
        reader.Read("cullMode", materialAsset.m_cullMode);
        reader.Read("isTransparent", materialAsset.m_isTransparent);
        reader.Read("depthTestEnabled", materialAsset.m_depthTestEnabled);
        reader.Read("depthWriteEnabled", materialAsset.m_depthWriteEnabled);
        return true;
    }

    bool CreateEmptyMaterialAssetFile(const std::filesystem::path& filePath, std::string_view materialName)
    {
        MaterialAsset materialAsset;
        materialAsset.m_name = std::string(materialName);
        return SaveMaterialAssetFile(filePath, materialAsset);
    }

    bool LoadMaterialAssetFile(const std::filesystem::path& filePath, MaterialAsset& materialAsset)
    {
        return IO::LoadFromFile(materialAsset, filePath, IO::FileFormat::Json);
    }

    bool SaveMaterialAssetFile(const std::filesystem::path& filePath, const MaterialAsset& materialAsset)
    {
        return IO::SaveToFile(materialAsset, filePath, IO::FileFormat::Json);
    }

    bool IsMaterialAssetFilePath(std::string_view filePath)
    {
        const std::string path(filePath);
        if (path.size() < std::char_traits<char>::length(MaterialAssetFileExtension))
        {
            return false;
        }

        return path.compare(path.size() - std::char_traits<char>::length(MaterialAssetFileExtension), std::char_traits<char>::length(MaterialAssetFileExtension), MaterialAssetFileExtension) == 0;
    }
} // namespace NexusEngine
