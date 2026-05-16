#include "EditorProjectRegistry.h"

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
        projectSettings.m_defaultScene = NexusEngine::IO::AssetReference{}; // No default scene

        const QString projectFilePath = GetProjectFilePath(project.m_rootPath);
        if (!NexusEngine::IO::SaveToFile(projectSettings, std::filesystem::path(projectFilePath.toStdWString()), NexusEngine::IO::FileFormat::Json))
        {
            return false;
        }

        const QString cmakeListsPath = QDir(project.m_rootPath).filePath(QStringLiteral("CMakeLists.txt"));
        const QString cmakePresetsPath = QDir(project.m_rootPath).filePath(QStringLiteral("CMakePresets.json"));
        const QString workspaceRootPath = QDir::cleanPath(QStringLiteral(NEXUS_EDITOR_CONTENT_ROOT)).replace('\\', '/');
        const QString cmakeListsContents = QStringLiteral(
            "cmake_minimum_required(VERSION 3.20)\n"
            "project(ProjectGameHost LANGUAGES CXX)\n\n"
            "set(CMAKE_CXX_STANDARD 20)\n"
            "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n\n"
            "set(NEXUS_EDITOR_MODE ON CACHE BOOL \"\" FORCE)\n\n"
            "if(NOT DEFINED NEXUS_PROJECT_OUTPUT_SUBDIR OR NEXUS_PROJECT_OUTPUT_SUBDIR STREQUAL \"\")\n"
            "    string(TOLOWER \"${CMAKE_BUILD_TYPE}\" NEXUS_PROJECT_BUILD_TYPE_LOWER)\n"
            "    if(BUILD_WEB)\n"
            "        set(NEXUS_PROJECT_OUTPUT_PLATFORM \"web\")\n"
            "    elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)\n"
            "        set(NEXUS_PROJECT_OUTPUT_PLATFORM \"x64\")\n"
            "    else()\n"
            "        set(NEXUS_PROJECT_OUTPUT_PLATFORM \"x86\")\n"
            "    endif()\n"
            "\n"
            "    if(NEXUS_PROJECT_BUILD_TYPE_LOWER STREQUAL \"release\")\n"
            "        set(NEXUS_PROJECT_OUTPUT_SUBDIR \"${NEXUS_PROJECT_OUTPUT_PLATFORM}-release\")\n"
            "    else()\n"
            "        set(NEXUS_PROJECT_OUTPUT_SUBDIR \"${NEXUS_PROJECT_OUTPUT_PLATFORM}-debug\")\n"
            "    endif()\n"
            "endif()\n"
            "\n"
            "set(CMAKE_RUNTIME_OUTPUT_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}/Binaries/${NEXUS_PROJECT_OUTPUT_SUBDIR}\")\n"
            "set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG \"${CMAKE_CURRENT_SOURCE_DIR}/Binaries/${NEXUS_PROJECT_OUTPUT_SUBDIR}\")\n"
            "set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE \"${CMAKE_CURRENT_SOURCE_DIR}/Binaries/${NEXUS_PROJECT_OUTPUT_SUBDIR}\")\n"
            "set(CMAKE_LIBRARY_OUTPUT_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}/Binaries/${NEXUS_PROJECT_OUTPUT_SUBDIR}\")\n"
            "set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG \"${CMAKE_CURRENT_SOURCE_DIR}/Binaries/${NEXUS_PROJECT_OUTPUT_SUBDIR}\")\n"
            "set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE \"${CMAKE_CURRENT_SOURCE_DIR}/Binaries/${NEXUS_PROJECT_OUTPUT_SUBDIR}\")\n"
            "set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}/Intermediate/${NEXUS_PROJECT_OUTPUT_SUBDIR}\")\n"
            "set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG \"${CMAKE_CURRENT_SOURCE_DIR}/Intermediate/${NEXUS_PROJECT_OUTPUT_SUBDIR}\")\n"
            "set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE \"${CMAKE_CURRENT_SOURCE_DIR}/Intermediate/${NEXUS_PROJECT_OUTPUT_SUBDIR}\")\n"
            "set(CMAKE_PDB_OUTPUT_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}/Intermediate/${NEXUS_PROJECT_OUTPUT_SUBDIR}\")\n"
            "set(CMAKE_PDB_OUTPUT_DIRECTORY_DEBUG \"${CMAKE_CURRENT_SOURCE_DIR}/Intermediate/${NEXUS_PROJECT_OUTPUT_SUBDIR}\")\n"
            "set(CMAKE_PDB_OUTPUT_DIRECTORY_RELEASE \"${CMAKE_CURRENT_SOURCE_DIR}/Intermediate/${NEXUS_PROJECT_OUTPUT_SUBDIR}\")\n\n"
            "set(CMAKE_OBJECT_PATH_MAX 128)\n"
            "set(NEXUS_BUILD_EDITOR OFF CACHE BOOL \"\" FORCE)\n"
            "set(NEXUS_BUILD_SAMPLE_PROJECTS OFF CACHE BOOL \"\" FORCE)\n"
            "set(NEXUS_WORKSPACE_SOURCE \"%1\")\n"
            "set(NEXUS_WORKSPACE_SUBST_DRIVE \"N:\")\n"
            "if(WIN32)\n"
            "    execute_process(COMMAND cmd /c subst ${NEXUS_WORKSPACE_SUBST_DRIVE} \"${NEXUS_WORKSPACE_SOURCE}\" OUTPUT_QUIET ERROR_QUIET)\n"
            "    if(EXISTS \"${NEXUS_WORKSPACE_SUBST_DRIVE}/CMakeLists.txt\")\n"
            "        set(NEXUS_WORKSPACE_SOURCE \"${NEXUS_WORKSPACE_SUBST_DRIVE}/\")\n"
            "    endif()\n"
            "endif()\n"
            "add_subdirectory(\"${NEXUS_WORKSPACE_SOURCE}\" W)\n\n"
            "add_library(ProjectGame SHARED\n"
            "    src/Game.cpp\n"
            ")\n\n"
            "target_link_libraries(ProjectGame PRIVATE NexusEngine)\n"
            "target_include_directories(ProjectGame PRIVATE\n"
            "    \"${NEXUS_WORKSPACE_SOURCE}/NexusEngine/src\"\n"
            ")\n"
            "target_compile_definitions(ProjectGame PRIVATE PROJECTGAME_EXPORTS)\n"
            "\n"
            "if(WIN32)\n"
            "    add_custom_command(TARGET ProjectGame POST_BUILD\n"
            "        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:ProjectGame> $<TARGET_FILE_DIR:ProjectGame>\n"
            "        COMMAND_EXPAND_LISTS\n"
            "        COMMENT \"Copying runtime DLLs for ProjectGame\")\n"
            "endif()\n")
            .arg(workspaceRootPath);
        if (!WriteTextFile(cmakeListsPath, cmakeListsContents))
        {
            return false;
        }

        const QString cmakePresetsContents = QStringLiteral(
            "{\n"
            "    \"version\": 3,\n"
            "    \"configurePresets\": [\n"
            "        {\n"
            "            \"name\": \"windows-base\",\n"
            "            \"hidden\": true,\n"
            "            \"generator\": \"Ninja\",\n"
            "            \"binaryDir\": \"${sourceDir}/Intermediate/${presetName}\",\n"
            "            \"installDir\": \"${sourceDir}/Intermediate/install/${presetName}\",\n"
            "            \"cacheVariables\": {\n"
            "                \"CMAKE_C_COMPILER\": \"cl.exe\",\n"
            "                \"CMAKE_CXX_COMPILER\": \"cl.exe\"\n"
            "            },\n"
            "            \"condition\": {\n"
            "                \"type\": \"equals\",\n"
            "                \"lhs\": \"${hostSystemName}\",\n"
            "                \"rhs\": \"Windows\"\n"
            "            }\n"
            "        },\n"
            "        {\n"
            "            \"name\": \"editor-debug\",\n"
            "            \"displayName\": \"Editor Debug\",\n"
            "            \"inherits\": \"windows-base\",\n"
            "            \"architecture\": {\n"
            "                \"value\": \"x64\",\n"
            "                \"strategy\": \"external\"\n"
            "            },\n"
            "            \"cacheVariables\": {\n"
            "                \"CMAKE_BUILD_TYPE\": \"Debug\",\n"
            "                \"NEXUS_PROJECT_OUTPUT_SUBDIR\": \"editor-debug\"\n"
            "            }\n"
            "        },\n"
            "        {\n"
            "            \"name\": \"editor-release\",\n"
            "            \"displayName\": \"Editor Release\",\n"
            "            \"inherits\": \"editor-debug\",\n"
            "            \"cacheVariables\": {\n"
            "                \"CMAKE_BUILD_TYPE\": \"Release\",\n"
            "                \"NEXUS_PROJECT_OUTPUT_SUBDIR\": \"editor-release\"\n"
            "            }\n"
            "        },\n"
            "        {\n"
            "            \"name\": \"x64-debug\",\n"
            "            \"displayName\": \"x64 Debug\",\n"
            "            \"inherits\": \"windows-base\",\n"
            "            \"architecture\": {\n"
            "                \"value\": \"x64\",\n"
            "                \"strategy\": \"external\"\n"
            "            },\n"
            "            \"cacheVariables\": {\n"
            "                \"CMAKE_BUILD_TYPE\": \"Debug\",\n"
            "                \"NEXUS_PROJECT_OUTPUT_SUBDIR\": \"x64-debug\"\n"
            "            }\n"
            "        },\n"
            "        {\n"
            "            \"name\": \"x64-release\",\n"
            "            \"displayName\": \"x64 Release\",\n"
            "            \"inherits\": \"x64-debug\",\n"
            "            \"cacheVariables\": {\n"
            "                \"CMAKE_BUILD_TYPE\": \"Release\",\n"
            "                \"NEXUS_PROJECT_OUTPUT_SUBDIR\": \"x64-release\"\n"
            "            }\n"
            "        },\n"
            "        {\n"
            "            \"name\": \"web-base\",\n"
            "            \"hidden\": true,\n"
            "            \"generator\": \"Ninja\",\n"
            "            \"binaryDir\": \"${sourceDir}/Intermediate/${presetName}\",\n"
            "            \"installDir\": \"${sourceDir}/Intermediate/install/${presetName}\",\n"
            "            \"cacheVariables\": {\n"
            "                \"CMAKE_TOOLCHAIN_FILE\": \"$env{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake\",\n"
            "                \"BUILD_WEB\": \"ON\"\n"
            "            },\n"
            "            \"condition\": {\n"
            "                \"type\": \"equals\",\n"
            "                \"lhs\": \"${hostSystemName}\",\n"
            "                \"rhs\": \"Windows\"\n"
            "            }\n"
            "        },\n"
            "        {\n"
            "            \"name\": \"web-debug\",\n"
            "            \"displayName\": \"Web Debug\",\n"
            "            \"inherits\": \"web-base\",\n"
            "            \"cacheVariables\": {\n"
            "                \"CMAKE_BUILD_TYPE\": \"Debug\",\n"
            "                \"NEXUS_PROJECT_OUTPUT_SUBDIR\": \"web-debug\"\n"
            "            }\n"
            "        },\n"
            "        {\n"
            "            \"name\": \"web-release\",\n"
            "            \"displayName\": \"Web Release\",\n"
            "            \"inherits\": \"web-debug\",\n"
            "            \"cacheVariables\": {\n"
            "                \"CMAKE_BUILD_TYPE\": \"Release\",\n"
            "                \"NEXUS_PROJECT_OUTPUT_SUBDIR\": \"web-release\"\n"
            "            }\n"
            "        }\n"
            "    ]\n"
            "}\n");
        if (!WriteTextFile(cmakePresetsPath, cmakePresetsContents))
        {
            return false;
        }

        const QString gameSourcePath = QDir(sourceDirectoryPath).filePath(QStringLiteral("Game.cpp"));
        const QString gameSourceContents = QStringLiteral(
            "#include <NexusEngine.h>\n"
            "#include <EngineComponentRegistration.h>\n"
            "#include <vector>\n\n"
            "#if defined(_WIN32)\n"
            "#if defined(PROJECTGAME_EXPORTS)\n"
            "#define PROJECTGAME_API __declspec(dllexport)\n"
            "#else\n"
            "#define PROJECTGAME_API __declspec(dllimport)\n"
            "#endif\n"
            "#else\n"
            "#define PROJECTGAME_API\n"
            "#endif\n\n"
            "namespace\n"
            "{\n"
            "    class ProjectGame final : public NexusEngine::IGameApp\n"
            "    {\n"
            "    public:\n"
            "        void RegisterComponentMetadata(flecs::world& world, NexusEngine::MetadataRegistry& metadataRegistry) override\n"
            "        {\n"
            "            NexusEngine::RegisterEngineComponentTypes(world);\n"
            "            REF(world);\n"
            "            REF(metadataRegistry);\n"
            "        }\n\n"
            "        void UnregisterComponentMetadata(NexusEngine::MetadataRegistry& metadataRegistry) override\n"
            "        {\n"
            "            REF(metadataRegistry);\n"
            "        }\n\n"
            "        std::vector<flecs::entity> RegisterSystems(NexusEngine::Engine& engine) override\n"
            "        {\n"
            "            REF(engine);\n"
            "            return {};\n"
            "        }\n"
            "    };\n"
            "}\n\n"
            "extern \"C\" PROJECTGAME_API NexusEngine::IGameApp* LoadGame()\n"
            "{\n"
            "    return new ProjectGame();\n"
            "}\n\n"
            "extern \"C\" PROJECTGAME_API void UnloadGame(NexusEngine::IGameApp* game)\n"
            "{\n"
            "    delete game;\n"
            "}\n");
        if (!WriteTextFile(gameSourcePath, gameSourceContents))
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
