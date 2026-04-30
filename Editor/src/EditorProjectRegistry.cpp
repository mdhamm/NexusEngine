#include "EditorProjectRegistry.h"

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
                projectObject[QStringLiteral("projectFilePath")] = project.m_projectFilePath;
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
            project.m_projectFilePath = projectObject[QStringLiteral("projectFilePath")].toString();

            if (project.m_projectFilePath.isEmpty())
            {
                project.m_projectFilePath = GetProjectFilePath(project.m_rootPath);
            }

            if (!project.m_rootPath.isEmpty() && QFileInfo::exists(project.m_rootPath) && QFileInfo::exists(project.m_projectFilePath))
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
        project.m_projectFilePath = GetProjectFilePath(project.m_rootPath);

        QFile file(project.m_projectFilePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            return false;
        }

        QJsonObject projectObject;
        projectObject[QStringLiteral("name")] = project.m_name;
        projectObject[QStringLiteral("rootPath")] = project.m_rootPath;
        file.write(QJsonDocument(projectObject).toJson(QJsonDocument::Indented));
        if (file.error() != QFileDevice::NoError)
        {
            return false;
        }

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
