#include "EditorSceneSerializer.h"

#include <filesystem/AssetReferenceRegistry.h>
#include <filesystem/AssetGuid.h>
#include <filesystem/FileIO.h>
#include <serialization/JsonSerializer.h>
#include <serialization/SceneSerialization.h>

#include <nlohmann/json.hpp>

#include <QFileInfo>

#include <cctype>
#include <filesystem>
#include <string_view>

namespace NexusEditor
{
    namespace
    {
        using json = nlohmann::json;

        std::filesystem::path ToFilesystemPath(const QString& path)
        {
            return std::filesystem::path(path.toStdWString());
        }

        std::string Trim(std::string_view text)
        {
            size_t start = 0;
            size_t end = text.size();
            while (start < end && std::isspace(static_cast<unsigned char>(text[start])) != 0)
            {
                ++start;
            }

            while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0)
            {
                --end;
            }

            return std::string(text.substr(start, end - start));
        }
    }

    bool SaveSceneToFile(const NexusEngine::Scene& scene, const QString& filePath)
    {
        return NexusEngine::IO::SaveToFile(scene, ToFilesystemPath(filePath), NexusEngine::IO::FileFormat::Json);
    }

    bool CreateEmptySceneFile(const QString& filePath, const QString& sceneName)
    {
        return CreateEmptySceneFile(filePath, sceneName, {});
    }

    bool CreateEmptySceneFile(const QString& filePath, const QString& sceneName, const QString& assetGuid)
    {
        const std::string trimmedGuid = Trim(assetGuid.toStdString());
        json sceneJson;
        NexusEngine::JsonSerializeWriter writer(sceneJson);

        writer.Write("guid", trimmedGuid.empty() ? NexusEngine::IO::CreateAssetGuid() : trimmedGuid);
        writer.Write("name", sceneName.toStdString());
        writer.BeginArray("entities");
        writer.EndArray();

        const std::string text = sceneJson.dump(4) + '\n';
        return NexusEngine::IO::WriteFileBytes(
            ToFilesystemPath(filePath),
            reinterpret_cast<const std::uint8_t*>(text.data()),
            text.size());
    }

    QString ReadSceneFileGuid(const QString& filePath)
    {
        return QString::fromStdString(NexusEngine::IO::ReadAssetReferenceGuidForPath(filePath.toStdString()));
    }

    QString EnsureSceneFileGuid(const QString& filePath)
    {
        return QString::fromStdString(NexusEngine::IO::EnsureAssetReferenceGuid(filePath.toStdString()));
    }

    bool IsSceneFilePath(const QString& filePath)
    {
        return NexusEngine::IO::IsSceneFilePath(filePath.toStdString());
    }
} // namespace NexusEditor
