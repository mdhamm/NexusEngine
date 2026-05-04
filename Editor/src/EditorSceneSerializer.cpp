#include "EditorSceneSerializer.h"

#include <SceneFileSerialization.h>

#include <filesystem>

namespace NexusEditor
{
    namespace
    {
        std::filesystem::path ToFilesystemPath(const QString& path)
        {
            return std::filesystem::path(path.toStdWString());
        }
    }

    bool SaveSceneToFile(const NexusEngine::Scene& scene, const QString& filePath)
    {
        return NexusEngine::SaveSceneToFile(scene, ToFilesystemPath(filePath));
    }

    bool SaveSceneToFile(const NexusEngine::Scene& scene, const QString& filePath, const QString& assetGuid)
    {
        return NexusEngine::SaveSceneToFile(scene, ToFilesystemPath(filePath), assetGuid.toStdString());
    }

    bool LoadSceneFromFile(NexusEngine::Scene& scene, const QString& filePath)
    {
        return NexusEngine::LoadSceneFromFile(scene, ToFilesystemPath(filePath));
    }

    bool CreateEmptySceneFile(const QString& filePath, const QString& sceneName)
    {
        return NexusEngine::CreateEmptySceneFile(ToFilesystemPath(filePath), sceneName.toStdString());
    }

    bool CreateEmptySceneFile(const QString& filePath, const QString& sceneName, const QString& assetGuid)
    {
        return NexusEngine::CreateEmptySceneFile(ToFilesystemPath(filePath), sceneName.toStdString(), assetGuid.toStdString());
    }

    QString ReadSceneFileGuid(const QString& filePath)
    {
        return QString::fromStdString(NexusEngine::ReadSceneFileGuid(ToFilesystemPath(filePath)));
    }

    QString EnsureSceneFileGuid(const QString& filePath)
    {
        return QString::fromStdString(NexusEngine::EnsureSceneFileGuid(ToFilesystemPath(filePath)));
    }

    bool IsSceneFilePath(const QString& filePath)
    {
        return NexusEngine::IsSceneFilePath(ToFilesystemPath(filePath));
    }
} // namespace NexusEditor
