#pragma once

#include "EditorProject.h"

namespace NexusEditor
{
    class EditorProjectBuilder
    {
    public:
        /// <summary>
        /// Creates a new project on disk from the editor's project templates.
        /// </summary>
        /// <param name="projectName">Name of the new project.</param>
        /// <param name="locationPath">Directory where the project folder should be created.</param>
        /// <param name="project">Created project information.</param>
        /// <returns>True if the project was created; otherwise false.</returns>
        static bool CreateProject(const QString& projectName, const QString& locationPath, EditorProject& project);
    };
} // namespace NexusEditor
