#include "EditorSceneSerializer.h"

#include <filesystem/AssetReferenceRegistry.h>
#include <filesystem/AssetGuid.h>
#include <filesystem/FileIO.h>
#include <serialization/JsonSerializer.h>
#include <serialization/SceneSerialization.h>

#include <nlohmann/json.hpp>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

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

    bool LoadSceneFromFile(NexusEngine::Scene& scene, const QString& filePath)
    {
        return NexusEngine::IO::LoadFromFile(scene, ToFilesystemPath(filePath), NexusEngine::IO::FileFormat::Json);
    }

    std::string SerializeSceneToJsonText(const NexusEngine::Scene& scene)
    {
        json sceneJson;
        NexusEngine::JsonSerializeWriter writer(sceneJson);
        writer.BeginObject();
        NexusEngine::Serialize(scene, writer);
        return sceneJson.dump(4) + '\n';
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
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
        {
            return {};
        }

        const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
        if (!document.isObject())
        {
            return {};
        }

        return document.object().value(QStringLiteral("guid")).toString().trimmed();
    }

    QString EnsureSceneFileGuid(const QString& filePath)
    {
        const QString existingGuid = ReadSceneFileGuid(filePath);
        if (!existingGuid.isEmpty())
        {
            return existingGuid;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
        {
            return {};
        }

        const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
        if (!document.isObject())
        {
            return {};
        }

        QJsonObject sceneObject = document.object();
        const QString guid = QString::fromStdString(NexusEngine::IO::CreateAssetGuid());
        sceneObject.insert(QStringLiteral("guid"), guid);

        const QFileInfo fileInfo(filePath);
        if (!QDir().mkpath(fileInfo.absolutePath()))
        {
            return {};
        }

        QFile outputFile(filePath);
        if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            return {};
        }

        outputFile.write(QJsonDocument(sceneObject).toJson(QJsonDocument::Indented));
        if (outputFile.error() != QFileDevice::NoError)
        {
            return {};
        }

        return guid;
    }

    bool IsSceneFilePath(const QString& filePath)
    {
        return filePath.endsWith(QStringLiteral(".nscene"), Qt::CaseInsensitive);
    }
} // namespace NexusEditor
