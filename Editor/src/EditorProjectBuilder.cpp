#include "EditorProjectBuilder.h"

#include "EditorProjectRegistry.h"

#include <ProjectSettings.h>
#include <filesystem/FileIO.h>
#include <serialization/ProjectSettingsSerialization.h>
#include <QDir>
#include <QFile>
#include <QHash>

#ifndef NEXUS_EDITOR_CONTENT_ROOT
#define NEXUS_EDITOR_CONTENT_ROOT "."
#endif

namespace NexusEditor
{
    namespace
    {
        bool WriteTextFile(const QString& filePath, const QString& content)
        {
            QFile file(filePath);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
            {
                return false;
            }

            file.write(content.toUtf8());
            return file.error() == QFileDevice::NoError;
        }

        bool WriteTemplatedFile(const QString& templatePath, const QString& destinationPath, const QHash<QString, QString>& replacements)
        {
            QFile file(templatePath);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                return false;
            }

            QString content = QString::fromUtf8(file.readAll());
            for (auto it = replacements.constBegin(); it != replacements.constEnd(); ++it)
            {
                content.replace(it.key(), it.value());
            }

            return WriteTextFile(destinationPath, content);
        }
    }

    bool EditorProjectBuilder::CreateProject(const QString& projectName, const QString& locationPath, EditorProject& project)
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
        project.m_requiresInitialBuild = true;

        const QString sourceDirectoryPath = QDir(project.m_rootPath).filePath(QStringLiteral("src"));
        if (!QDir().mkpath(sourceDirectoryPath))
        {
            return false;
        }

        NexusEngine::ProjectSettings projectSettings{};
        projectSettings.m_name = project.m_name.toStdString();
        projectSettings.m_defaultScene = NexusEngine::IO::AssetReference{};

        const QString projectFilePath = EditorProjectRegistry::GetProjectFilePath(project.m_rootPath);
        if (!NexusEngine::IO::SaveToFile(projectSettings, std::filesystem::path(projectFilePath.toStdWString()), NexusEngine::IO::FileFormat::Json))
        {
            return false;
        }

        const QString templateRootPath = QDir(QStringLiteral(NEXUS_EDITOR_CONTENT_ROOT)).filePath(QStringLiteral("Editor/templates/project"));
        const QString workspaceRootPath = QDir::cleanPath(QStringLiteral(NEXUS_EDITOR_CONTENT_ROOT)).replace('\\', '/');
        const QString projectTargetName = project.m_name.trimmed().isEmpty() ? QStringLiteral("ProjectGame") : project.m_name.trimmed();
        const QString sanitizedProjectTargetName = QString(projectTargetName).replace('"', QStringLiteral(""));
        const QString normalizedProjectRootPath = QDir::cleanPath(project.m_rootPath).replace('\\', '/');

        const QHash<QString, QString> replacements = {
            { QStringLiteral("{{PROJECT_NAME}}"), sanitizedProjectTargetName },
            { QStringLiteral("{{WORKSPACE_ROOT}}"), workspaceRootPath },
            { QStringLiteral("{{PROJECT_ROOT}}"), normalizedProjectRootPath }
        };

        if (!WriteTemplatedFile(QDir(templateRootPath).filePath(QStringLiteral("CMakeLists.txt.in")), QDir(project.m_rootPath).filePath(QStringLiteral("CMakeLists.txt")), replacements))
        {
            return false;
        }

        if (!WriteTemplatedFile(QDir(templateRootPath).filePath(QStringLiteral("CMakePresets.json.in")), QDir(project.m_rootPath).filePath(QStringLiteral("CMakePresets.json")), replacements))
        {
            return false;
        }

        if (!WriteTemplatedFile(QDir(templateRootPath).filePath(QStringLiteral("launch.vs.json.in")), QDir(project.m_rootPath).filePath(QStringLiteral("launch.vs.json")), replacements))
        {
            return false;
        }

        if (!WriteTemplatedFile(QDir(templateRootPath).filePath(QStringLiteral("src/Game.cpp.in")), QDir(sourceDirectoryPath).filePath(QStringLiteral("Game.cpp")), replacements))
        {
            return false;
        }

        if (!WriteTemplatedFile(QDir(templateRootPath).filePath(QStringLiteral("src/main_win32.cpp.in")), QDir(sourceDirectoryPath).filePath(QStringLiteral("main_win32.cpp")), replacements))
        {
            return false;
        }

        return EditorProjectRegistry::SaveRecentProject(project);
    }
} // namespace NexusEditor
