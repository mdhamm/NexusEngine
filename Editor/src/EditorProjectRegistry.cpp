#include "EditorProjectRegistry.h"

#include "EditorProjectBuilder.h"

#include <ProjectSettings.h>
#include <filesystem/FileIO.h>
#include <serialization/ProjectSettingsSerialization.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace NexusEditor
{
    namespace
    {
        QString GetRecentProjectsFilePath()
        {
            const QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            QDir().mkpath(appDataPath);
            return QDir(appDataPath).filePath(QStringLiteral("projects.json"));
        }

        bool SaveRecentProjects(const QVector<EditorProject>& projects)
        {
            QJsonArray projectArray;
            for (const EditorProject& project : projects)
            {
                QJsonObject projectObject;
                projectObject[QStringLiteral("name")] = project.m_name;
                projectObject[QStringLiteral("rootPath")] = project.m_rootPath;
                projectObject[QStringLiteral("requiresInitialBuild")] = project.m_requiresInitialBuild;
                projectArray.append(projectObject);
            }

            QFile file(GetRecentProjectsFilePath());
            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            {
                return false;
            }

            file.write(QJsonDocument(projectArray).toJson(QJsonDocument::Indented));
            return file.error() == QFileDevice::NoError;
        }
    }

    QVector<EditorProject> EditorProjectRegistry::LoadRecentProjects()
    {
        QVector<EditorProject> projects;

        QFile file(GetRecentProjectsFilePath());
        if (!file.open(QIODevice::ReadOnly))
        {
            return projects;
        }

        const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
        if (!document.isArray())
        {
            return projects;
        }

        const QJsonArray projectArray = document.array();
        for (const QJsonValue& projectValue : projectArray)
        {
            if (!projectValue.isObject())
            {
                continue;
            }

            const QJsonObject projectObject = projectValue.toObject();
            EditorProject project;
            project.m_name = projectObject[QStringLiteral("name")].toString();
            project.m_rootPath = projectObject[QStringLiteral("rootPath")].toString();
            project.m_requiresInitialBuild = projectObject[QStringLiteral("requiresInitialBuild")].toBool(false);

            if (!project.m_rootPath.isEmpty() && QFileInfo::exists(project.m_rootPath))
            {
                NexusEngine::ProjectSettings projectSettings;
                const QString projectFilePath = GetProjectFilePath(project.m_rootPath);
                if (NexusEngine::IO::LoadFromFile(projectSettings, std::filesystem::path(projectFilePath.toStdWString()), NexusEngine::IO::FileFormat::Json))
                {
                    if (!projectSettings.m_name.empty())
                    {
                        project.m_name = QString::fromStdString(projectSettings.m_name);
                    }

                    project.m_defaultScene = projectSettings.m_defaultScene;
                }

                projects.push_back(project);
            }
        }

        return projects;
    }

    bool EditorProjectRegistry::CreateProject(const QString& projectName, const QString& locationPath, EditorProject& project)
    {
        return EditorProjectBuilder::CreateProject(projectName, locationPath, project);
    }

    bool EditorProjectRegistry::AddRecentProject(const EditorProject& project)
    {
        QVector<EditorProject> projects = LoadRecentProjects();

        for (int i = 0; i < projects.size(); ++i)
        {
            if (projects[i].m_rootPath.compare(project.m_rootPath, Qt::CaseInsensitive) == 0)
            {
                projects.removeAt(i);
                break;
            }
        }

        projects.prepend(project);
        return SaveRecentProjects(projects);
    }

    QString EditorProjectRegistry::GetProjectFilePath(const QString& projectRootPath)
    {
        return QDir(projectRootPath).filePath(QStringLiteral("NexusProject.json"));
    }

    bool EditorProjectRegistry::SaveRecentProject(const EditorProject& project)
    {
        return AddRecentProject(project);
    }
} // namespace NexusEditor
