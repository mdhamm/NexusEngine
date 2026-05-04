#include "EditorSceneSerializer.h"

#include <io/AssetGuid.h>
#include <io/FileIO.h>
#include <io/SceneIO.h>
#include <serialization/SceneSerialization.h>
#include <serialization/JsonSerializer.h>

#include <nlohmann/json.hpp>

#include <QFileInfo>

#include <cctype>
#include <sstream>
#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

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

        bool HasSceneFileExtension(const QString& filePath)
        {
            return QFileInfo(filePath).suffix().compare(QStringLiteral("nscene"), Qt::CaseInsensitive) == 0;
        }
    }

    bool SaveSceneToFile(const NexusEngine::Scene& scene, const QString& filePath)
    {
        return NexusEngine::IO::SaveSceneToFile(
            scene,
            ToFilesystemPath(filePath),
            [](const NexusEngine::Scene& sourceScene, NexusEngine::ISerializeWriter& writer)
            {
                NexusEngine::SerializeScene(sourceScene, writer);
                return true;
            },
            [](std::vector<std::uint8_t>& bytes, const NexusEngine::Scene& sourceScene, const NexusEngine::SerializeSceneMethod& serializeMethod)
            {
                json sceneJson;
                NexusEngine::JsonSerializeWriter writer(sceneJson);
                writer.BeginObject();

                if (!serializeMethod(sourceScene, writer))
                {
                    return false;
                }

                const std::string text = sceneJson.dump(4) + '\n';
                bytes.assign(text.begin(), text.end());
                return true;
            });
    }

    bool LoadSceneFromFile(NexusEngine::Scene& scene, const QString& filePath)
    {
        return NexusEngine::IO::LoadSceneFromFile(
            scene,
            ToFilesystemPath(filePath),
            [](NexusEngine::Scene& destinationScene, NexusEngine::ISerializeReader& reader)
            {
                return NexusEngine::DeserializeScene(destinationScene, reader);
            },
            [](const std::vector<std::uint8_t>& bytes, NexusEngine::Scene& destinationScene, const NexusEngine::DeserializeSceneMethod& deserializeMethod)
            {
                json document;
                try
                {
                    document = json::parse(bytes.begin(), bytes.end());
                }
                catch (...)
                {
                    return false;
                }

                if (!document.is_object())
                {
                    return false;
                }

                NexusEngine::JsonSerializeReader reader(document);
                return deserializeMethod(destinationScene, reader);
            });
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
        const std::optional<std::vector<std::uint8_t>> fileBytes = NexusEngine::IO::ReadFileBytes(ToFilesystemPath(filePath));
        if (!fileBytes)
        {
            return {};
        }

        try
        {
            const json document = json::parse(fileBytes->begin(), fileBytes->end());
            return document.is_object()
                ? QString::fromStdString(Trim(document.value("guid", std::string{})))
                : QString{};
        }
        catch (...)
        {
            return {};
        }
    }

    QString EnsureSceneFileGuid(const QString& filePath)
    {
        const std::filesystem::path filesystemPath = ToFilesystemPath(filePath);
        const std::optional<std::vector<std::uint8_t>> fileBytes = NexusEngine::IO::ReadFileBytes(filesystemPath);
        if (!fileBytes)
        {
            return {};
        }

        json document;
        try
        {
            document = json::parse(fileBytes->begin(), fileBytes->end());
        }
        catch (...)
        {
            return {};
        }

        if (!document.is_object())
        {
            return {};
        }

        const std::string existingGuid = Trim(document.value("guid", std::string{}));
        if (!existingGuid.empty())
        {
            return QString::fromStdString(existingGuid);
        }

        const std::string assetGuid = NexusEngine::IO::CreateAssetGuid();
        document["guid"] = assetGuid;
        const std::string text = document.dump(4) + '\n';
        return NexusEngine::IO::WriteFileBytes(
            filesystemPath,
            reinterpret_cast<const std::uint8_t*>(text.data()),
            text.size())
            ? QString::fromStdString(assetGuid)
            : QString{};
    }

    bool IsSceneFilePath(const QString& filePath)
    {
        return HasSceneFileExtension(filePath);
    }
} // namespace NexusEditor
