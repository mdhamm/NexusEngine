#pragma once

#include "FileIO.h"
#include "ProjectSettings.h"

#include <filesystem>
#include <string>

namespace NexusEngine::IO
{

    /// <summary>
    /// Saves project metadata to a file using the requested file format.
    /// </summary>
    /// <param name="project">Project metadata to save.</param>
    /// <param name="filePath">Destination project file path.</param>
    /// <param name="fileFormat">Structured file format to write.</param>
    /// <returns>True if the project file was written; otherwise false.</returns>
    bool SaveProjectToFile(const ProjectSettings& project, const std::filesystem::path& filePath, FileFormat fileFormat = FileFormat::Json);

    /// <summary>
    /// Loads project metadata from a file using the requested file format.
    /// </summary>
    /// <param name="project">Project metadata to populate.</param>
    /// <param name="filePath">Source project file path.</param>
    /// <param name="fileFormat">Structured file format to read.</param>
    /// <returns>True if the project file was loaded; otherwise false.</returns>
    bool LoadProjectFromFile(ProjectSettings& project, const std::filesystem::path& filePath, FileFormat fileFormat = FileFormat::Json);
} // namespace NexusEngine::IO
