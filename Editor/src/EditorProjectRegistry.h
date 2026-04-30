#pragma once

#include "EditorProject.h"

#include <QVector>

namespace NexusEditor
{
    class EditorProjectRegistry
    {
    public:
        /// <summary>
        /// Loads the recent editor projects from local storage.
        /// </summary>
        /// <returns>The known recent projects.</returns>
        static QVector<EditorProject> LoadRecentProjects();

        /// <summary>
        /// Creates a new project on disk and adds it to the recent projects list.
        /// </summary>
        /// <param name="projectName">Name of the new project.</param>
        /// <param name="locationPath">Directory where the project folder should be created.</param>
        /// <param name="project">Created project information.</param>
        /// <returns>True if the project was created; otherwise false.</returns>
        static bool CreateProject(const QString& projectName, const QString& locationPath, EditorProject& project);

        /// <summary>
        /// Adds or updates a recent project entry.
        /// </summary>
        /// <param name="project">Project to store in the recent project list.</param>
        /// <returns>True if the recent project list was updated; otherwise false.</returns>
        static bool AddRecentProject(const EditorProject& project);

        /// <summary>
        /// Returns the project metadata file path for a project root.
        /// </summary>
        /// <param name="projectRootPath">Project root directory.</param>
        /// <returns>The metadata file path.</returns>
        static QString GetProjectFilePath(const QString& projectRootPath);
    };
} // namespace NexusEditor
