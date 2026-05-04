#pragma once

#include "Scene.h"

#include "FileIO.h"

#include <filesystem>

namespace NexusEngine::IO
{
    /// <summary>
    /// Saves a scene to a file using the requested file format.
    /// </summary>
    /// <param name="scene">Scene to save.</param>
    /// <param name="filePath">Destination scene file path.</param>
    /// <param name="fileFormat">Structured file format to write.</param>
    /// <returns>True if the scene file was written; otherwise false.</returns>
    bool SaveSceneToFile(const Scene& scene, const std::filesystem::path& filePath, FileFormat fileFormat);

    /// <summary>
    /// Loads a scene from a file using the requested file format.
    /// </summary>
    /// <param name="scene">Scene to populate.</param>
    /// <param name="filePath">Source scene file path.</param>
    /// <param name="fileFormat">Structured file format to read.</param>
    /// <returns>True if the scene file was loaded; otherwise false.</returns>
    bool LoadSceneFromFile(Scene& scene, const std::filesystem::path& filePath, FileFormat fileFormat);
} // namespace NexusEngine::IO
