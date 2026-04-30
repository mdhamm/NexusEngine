#include "AssetFileReference.h"

#include "EditorSceneSerializer.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

namespace NexusEditor
{
    struct AssetFileReferenceData
    {
        QString m_guid;
        QString m_path;
        QString m_projectRootPath;
    };

    namespace
    {
        QString FindProjectRootPath(const QString& path)
        {
            QFileInfo fileInfo(path);
            QDir directory = fileInfo.isDir() ? QDir(fileInfo.absoluteFilePath()) : fileInfo.absoluteDir();
            while (directory.exists())
            {
                if (QFileInfo::exists(directory.filePath(QStringLiteral("NexusProject.json"))))
                {
                    return QDir::cleanPath(directory.absolutePath());
                }

                if (!directory.cdUp())
                {
                    break;
                }
            }

            return QDir::cleanPath(fileInfo.isDir() ? fileInfo.absoluteFilePath() : fileInfo.absolutePath());
        }

        QString GetAssetReferenceDirectoryPath(const QString& projectRootPath)
        {
            return QDir(projectRootPath).filePath(QStringLiteral(".nexus/assetrefs"));
        }

        QString GetAssetReferenceFilePath(const QString& projectRootPath, const QString& guid)
        {
            return QDir(GetAssetReferenceDirectoryPath(projectRootPath)).filePath(guid + QStringLiteral(".json"));
        }

        QString ToStoredAssetPath(const QString& projectRootPath, const QString& assetPath)
        {
            const QString cleanProjectRootPath = QDir::cleanPath(projectRootPath);
            const QString cleanAssetPath = QDir::cleanPath(assetPath);
            const QString relativePath = QDir(cleanProjectRootPath).relativeFilePath(cleanAssetPath);
            return relativePath.startsWith(QStringLiteral("..")) ? cleanAssetPath : relativePath;
        }

        QString ToAbsoluteAssetPath(const QString& projectRootPath, const QString& storedPath)
        {
            if (storedPath.isEmpty())
            {
                return {};
            }

            const QFileInfo fileInfo(storedPath);
            return fileInfo.isAbsolute()
                ? QDir::cleanPath(storedPath)
                : QDir(projectRootPath).filePath(storedPath);
        }

        QString ReadReferencedPath(const QString& projectRootPath, const QString& guid)
        {
            QFile file(GetAssetReferenceFilePath(projectRootPath, guid));
            if (!file.open(QIODevice::ReadOnly))
            {
                return {};
            }

            const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
            if (!document.isObject())
            {
                return {};
            }

            return QDir::cleanPath(ToAbsoluteAssetPath(projectRootPath, document.object()["path"].toString().trimmed()));
        }

        bool WriteReferenceFile(const QString& projectRootPath, const QString& guid, const QString& assetPath)
        {
            if (projectRootPath.isEmpty() || guid.isEmpty() || assetPath.isEmpty())
            {
                return false;
            }

            const QString referenceDirectoryPath = GetAssetReferenceDirectoryPath(projectRootPath);
            if (!QDir().mkpath(referenceDirectoryPath))
            {
                return false;
            }

            QFile file(GetAssetReferenceFilePath(projectRootPath, guid));
            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            {
                return false;
            }

            QJsonObject referenceObject;
            referenceObject["guid"] = guid;
            referenceObject["path"] = ToStoredAssetPath(projectRootPath, assetPath);
            file.write(QJsonDocument(referenceObject).toJson(QJsonDocument::Indented));
            return file.error() == QFileDevice::NoError;
        }

        class AssetFileReferenceRegistry final
        {
        public:
            static AssetFileReferenceRegistry& Instance()
            {
                static AssetFileReferenceRegistry s_instance;
                return s_instance;
            }

            std::shared_ptr<AssetFileReferenceData> BindPath(const QString& path)
            {
                const QString cleanPath = QDir::cleanPath(path);
                if (cleanPath.isEmpty())
                {
                    return {};
                }

                const QString projectRootPath = FindProjectRootPath(cleanPath);

                QString guid = ReadGuidForPath(cleanPath);
                if (guid.isEmpty())
                {
                    guid = IsSceneFilePath(cleanPath)
                        ? EnsureSceneFileGuid(cleanPath)
                        : QUuid::createUuid().toString(QUuid::WithoutBraces);
                }

                if (guid.isEmpty())
                {
                    return {};
                }

                const auto existingByGuid = FindByGuid(guid);
                if (existingByGuid)
                {
                    Reindex(existingByGuid, cleanPath, projectRootPath);
                    return existingByGuid;
                }

                auto data = std::make_shared<AssetFileReferenceData>();
                data->m_guid = guid;
                data->m_path = cleanPath;
                data->m_projectRootPath = projectRootPath;
                m_recordsByGuid.insert(guid, data);
                m_guidByPath.insert(cleanPath, guid);
                WriteReferenceFile(projectRootPath, guid, cleanPath);
                return data;
            }

            void SetPath(const std::shared_ptr<AssetFileReferenceData>& data, const QString& path)
            {
                if (!data)
                {
                    return;
                }

                const QString cleanPath = QDir::cleanPath(path);
                if (cleanPath.isEmpty())
                {
                    return;
                }

                const QString projectRootPath = FindProjectRootPath(cleanPath);
                Reindex(data, cleanPath, projectRootPath);
            }

            bool UpdatePath(const std::shared_ptr<AssetFileReferenceData>& data, const QString& oldPath, const QString& newPath)
            {
                if (!data)
                {
                    return false;
                }

                const QString cleanOldPath = QDir::cleanPath(oldPath);
                const QString cleanCurrentPath = QDir::cleanPath(data->m_path);
                if (cleanCurrentPath.compare(cleanOldPath, Qt::CaseInsensitive) != 0)
                {
                    return false;
                }

                Reindex(data, newPath, FindProjectRootPath(newPath));
                return true;
            }

            void NotifyPathChanged(const QString& oldPath, const QString& newPath)
            {
                const QString cleanOldPath = QDir::cleanPath(oldPath);
                const QString cleanNewPath = QDir::cleanPath(newPath);
                QString guid = FindGuidByPath(cleanOldPath);
                if (guid.isEmpty())
                {
                    guid = ReadGuidForPath(cleanNewPath);
                }

                if (guid.isEmpty())
                {
                    return;
                }

                auto data = FindByGuid(guid);
                if (!data)
                {
                    data = std::make_shared<AssetFileReferenceData>();
                    data->m_guid = guid;
                    m_recordsByGuid.insert(guid, data);
                }

                const QString projectRootPath = FindProjectRootPath(cleanNewPath.isEmpty() ? cleanOldPath : cleanNewPath);
                if (data)
                {
                    Reindex(data, cleanNewPath, projectRootPath);
                }
            }

            bool ResolveWithinProject(const std::shared_ptr<AssetFileReferenceData>& data, const QString& projectRootPath)
            {
                if (!data || data->m_guid.isEmpty())
                {
                    return false;
                }

                const QString resolvedProjectRootPath = projectRootPath.isEmpty()
                    ? data->m_projectRootPath
                    : QDir::cleanPath(projectRootPath);

                const QString referencedPath = ReadReferencedPath(resolvedProjectRootPath, data->m_guid);
                if (!referencedPath.isEmpty())
                {
                    const QFileInfo referencedFileInfo(referencedPath);
                    if (referencedFileInfo.exists() || referencedFileInfo.dir().exists())
                    {
                        Reindex(data, referencedPath, resolvedProjectRootPath);
                        return true;
                    }
                }

                const QFileInfo currentFileInfo(data->m_path);
                if (currentFileInfo.exists())
                {
                    return true;
                }

                if (data->m_guid.isEmpty())
                {
                    return false;
                }

                QStringList nameFilters;
                if (IsSceneFilePath(data->m_path))
                {
                    nameFilters.push_back(QStringLiteral("*.nscene"));
                }
                else if (!currentFileInfo.suffix().isEmpty())
                {
                    nameFilters.push_back(QStringLiteral("*.%1").arg(currentFileInfo.suffix()));
                }

                QDirIterator iterator(
                    resolvedProjectRootPath,
                    nameFilters,
                    QDir::Files,
                    QDirIterator::Subdirectories);
                while (iterator.hasNext())
                {
                    const QString candidatePath = QDir::cleanPath(iterator.next());
                    const QString candidateGuid = ReadGuidForPath(candidatePath);
                    if (candidateGuid.compare(data->m_guid, Qt::CaseInsensitive) == 0)
                    {
                        Reindex(data, candidatePath, resolvedProjectRootPath);
                        return true;
                    }
                }

                return currentFileInfo.dir().exists();
            }

        private:
            std::shared_ptr<AssetFileReferenceData> FindByGuid(const QString& guid)
            {
                auto it = m_recordsByGuid.find(guid);
                if (it == m_recordsByGuid.end())
                {
                    return {};
                }

                const auto data = it.value().lock();
                if (!data)
                {
                    m_recordsByGuid.erase(it);
                }
                return data;
            }

            std::shared_ptr<AssetFileReferenceData> FindByPath(const QString& path)
            {
                auto it = m_guidByPath.find(path);
                if (it == m_guidByPath.end())
                {
                    return {};
                }

                const auto data = FindByGuid(it.value());
                if (!data)
                {
                    m_guidByPath.erase(it);
                }
                return data;
            }

            QString FindGuidByPath(const QString& path) const
            {
                const auto it = m_guidByPath.find(path);
                return it == m_guidByPath.end() ? QString{} : it.value();
            }

            QString ReadGuidForPath(const QString& path) const
            {
                return IsSceneFilePath(path) ? ReadSceneFileGuid(path) : QString{};
            }

            void Reindex(const std::shared_ptr<AssetFileReferenceData>& data, const QString& path, const QString& projectRootPath)
            {
                if (!data)
                {
                    return;
                }

                const QString cleanPath = QDir::cleanPath(path);
                if (!data->m_path.isEmpty())
                {
                    m_guidByPath.remove(QDir::cleanPath(data->m_path));
                }

                data->m_path = cleanPath;
                data->m_projectRootPath = QDir::cleanPath(projectRootPath);
                if (!data->m_guid.isEmpty() && !cleanPath.isEmpty())
                {
                    m_recordsByGuid.insert(data->m_guid, data);
                    m_guidByPath.insert(cleanPath, data->m_guid);
                    WriteReferenceFile(data->m_projectRootPath, data->m_guid, cleanPath);
                }
            }

            QHash<QString, std::weak_ptr<AssetFileReferenceData>> m_recordsByGuid;
            QHash<QString, QString> m_guidByPath;
        };
    }

    AssetFileReference AssetFileReference::FromPath(const QString& path)
    {
        return AssetFileReference(AssetFileReferenceRegistry::Instance().BindPath(path));
    }

    void AssetFileReference::NotifyPathChanged(const QString& oldPath, const QString& newPath)
    {
        AssetFileReferenceRegistry::Instance().NotifyPathChanged(oldPath, newPath);
    }

    bool AssetFileReference::IsEmpty() const
    {
        return !m_data || m_data->m_guid.isEmpty();
    }

    QString AssetFileReference::GetPath() const
    {
        return m_data ? m_data->m_path : QString{};
    }

    QString AssetFileReference::GetGuid() const
    {
        return m_data ? m_data->m_guid : QString{};
    }

    void AssetFileReference::BindToPath(const QString& path)
    {
        m_data = AssetFileReferenceRegistry::Instance().BindPath(path);
    }

    void AssetFileReference::SetPath(const QString& path)
    {
        if (!m_data)
        {
            BindToPath(path);
            return;
        }

        AssetFileReferenceRegistry::Instance().SetPath(m_data, path);
    }

    bool AssetFileReference::UpdatePath(const QString& oldPath, const QString& newPath)
    {
        return AssetFileReferenceRegistry::Instance().UpdatePath(m_data, oldPath, newPath);
    }

    bool AssetFileReference::ResolveWithinProject(const QString& projectRootPath) const
    {
        return AssetFileReferenceRegistry::Instance().ResolveWithinProject(m_data, projectRootPath);
    }

    AssetFileReference::AssetFileReference(std::shared_ptr<AssetFileReferenceData> data)
        : m_data(std::move(data))
    {
    }
} // namespace NexusEditor
