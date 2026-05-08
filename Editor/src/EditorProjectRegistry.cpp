#include "EditorProjectRegistry.h"

#include <ProjectSettings.h>
#include <QDir>
#include <QJsonArray>
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

            if (!project.m_rootPath.isEmpty() && QFileInfo::exists(project.m_rootPath))
            {
                projects.push_back(project);
            }
        }

        return projects;
    }

    bool EditorProjectRegistry::CreateProject(const QString& projectName, const QString& locationPath, EditorProject& project)
    {
        if (projectName.trimmed().isEmpty() || locationPath.trimmed().isEmpty())
        {
            return false;
        }

        const QString projectRootPath = QDir(locationPath).filePath(projectName.trimmed());
        if (!QDir().mkpath(projectRootPath))
        {
            return false;
        }

        project.m_name = projectName.trimmed();
        project.m_rootPath = QDir::cleanPath(projectRootPath);

        NexusEngine::ProjectSettings projectSettings{};
        projectSettings.m_name = project.m_name.toStdString();
        projectSettings.m_defaultScene = NexusEngine::IO::AssetReference{}; // No default scene

        return AddRecentProject(project);
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
} // namespace NexusEditor
