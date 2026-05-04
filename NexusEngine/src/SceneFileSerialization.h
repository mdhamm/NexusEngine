#pragma once

#include "Scene.h"

#include <filesystem>
#include <string>
#include <string_view>

namespace NexusEngine
{
    /// <summary>
    /// Saves a scene to a scene asset file.
    /// </summary>
    /// <param name="scene">Scene to save.</param>
    /// <param name="filePath">Destination scene file path.</param>
    /// <returns>True if the scene file was written; otherwise false.</returns>
    bool SaveSceneToFile(const Scene& scene, const std::filesystem::path& filePath);

    /// <summary>
    /// Saves a scene to a scene asset file using a stable asset guid.
    /// </summary>
    /// <param name="scene">Scene to save.</param>
    /// <param name="filePath">Destination scene file path.</param>
    /// <param name="assetGuid">Stable guid stored with the asset.</param>
    /// <returns>True if the scene file was written; otherwise false.</returns>
    bool SaveSceneToFile(const Scene& scene, const std::filesystem::path& filePath, std::string_view assetGuid);

    /// <summary>
    /// Loads a scene from a scene asset file.
    /// </summary>
    /// <param name="scene">Scene to populate.</param>
    /// <param name="filePath">Source scene file path.</param>
    /// <returns>True if the scene file was loaded; otherwise false.</returns>
    bool LoadSceneFromFile(Scene& scene, const std::filesystem::path& filePath);

    /// <summary>
    /// Creates an empty scene asset file on disk.
    /// </summary>
    /// <param name="filePath">Destination scene file path.</param>
    /// <param name="sceneName">Scene name stored in the asset.</param>
    /// <returns>True if the scene file was created; otherwise false.</returns>
    bool CreateEmptySceneFile(const std::filesystem::path& filePath, std::string_view sceneName);

    /// <summary>
    /// Creates an empty scene asset file on disk using a stable asset guid.
    /// </summary>
    /// <param name="filePath">Destination scene file path.</param>
    /// <param name="sceneName">Scene name stored in the asset.</param>
    /// <param name="assetGuid">Stable guid stored with the asset.</param>
    /// <returns>True if the scene file was created; otherwise false.</returns>
    bool CreateEmptySceneFile(const std::filesystem::path& filePath, std::string_view sceneName, std::string_view assetGuid);

    /// <summary>
    /// Reads the stable guid stored in a scene asset file.
    /// </summary>
    /// <param name="filePath">Scene file path.</param>
    /// <returns>The stored guid, or an empty string if none exists.</returns>
    std::string ReadSceneFileGuid(const std::filesystem::path& filePath);

    /// <summary>
    /// Ensures a scene asset file contains a stable guid and returns it.
    /// </summary>
    /// <param name="filePath">Scene file path.</param>
    /// <returns>The guid stored in the scene asset after repair, or an empty string if the file could not be updated.</returns>
    std::string EnsureSceneFileGuid(const std::filesystem::path& filePath);

    /// <summary>
    /// Returns whether a path points to a scene asset file.
    /// </summary>
    /// <param name="filePath">Path to inspect.</param>
    /// <returns>True if the path uses the scene asset extension; otherwise false.</returns>
    bool IsSceneFilePath(const std::filesystem::path& filePath);
} // namespace NexusEngine
