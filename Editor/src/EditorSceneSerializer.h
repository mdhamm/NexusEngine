#pragma once

#include <QString>

#ifdef emit
#undef emit
#endif

namespace NexusEngine
{
    struct Scene;
}

namespace NexusEditor
{
    /// <summary>
    /// Saves a scene to a file using reflected component properties.
    /// </summary>
    /// <param name="scene">Scene to save.</param>
    /// <param name="filePath">Destination file path.</param>
    /// <returns>True if the scene was saved; otherwise false.</returns>
    bool SaveSceneToFile(const NexusEngine::Scene& scene, const QString& filePath);

    /// <summary>
    /// Loads a scene from a file using reflected component properties.
    /// </summary>
    /// <param name="scene">Scene to populate from disk.</param>
    /// <param name="filePath">Source file path.</param>
    /// <returns>True if the scene was loaded; otherwise false.</returns>
    bool LoadSceneFromFile(NexusEngine::Scene& scene, const QString& filePath);

    /// <summary>
    /// Serializes a scene to JSON text using reflected component properties.
    /// </summary>
    /// <param name="scene">Scene to serialize.</param>
    /// <returns>The serialized scene JSON text.</returns>
    std::string SerializeSceneToJsonText(const NexusEngine::Scene& scene);

    /// <summary>
    /// Creates an empty scene asset file on disk.
    /// </summary>
    /// <param name="filePath">Destination scene asset path.</param>
    /// <param name="sceneName">Scene name stored in the asset.</param>
    /// <returns>True if the scene file was created; otherwise false.</returns>
    bool CreateEmptySceneFile(const QString& filePath, const QString& sceneName);

    /// <summary>
    /// Creates an empty scene asset file on disk with a stable guid.
    /// </summary>
    /// <param name="filePath">Destination scene asset path.</param>
    /// <param name="sceneName">Scene name stored in the asset.</param>
    /// <param name="assetGuid">Stable guid stored with the asset.</param>
    /// <returns>True if the scene file was created; otherwise false.</returns>
    bool CreateEmptySceneFile(const QString& filePath, const QString& sceneName, const QString& assetGuid);

    /// <summary>
    /// Reads the stable guid stored in a scene asset file.
    /// </summary>
    /// <param name="filePath">Scene asset path.</param>
    /// <returns>The stored guid, or an empty string if none exists.</returns>
    QString ReadSceneFileGuid(const QString& filePath);

    /// <summary>
    /// Ensures a scene asset file contains a stable guid and returns it.
    /// </summary>
    /// <param name="filePath">Scene asset path.</param>
    /// <returns>The guid stored in the scene asset after repair, or an empty string if the file could not be updated.</returns>
    QString EnsureSceneFileGuid(const QString& filePath);

    /// <summary>
    /// Returns whether a path points to a scene asset file.
    /// </summary>
    /// <param name="filePath">Path to inspect.</param>
    /// <returns>True if the path uses the editor scene asset extension; otherwise false.</returns>
    bool IsSceneFilePath(const QString& filePath);
} // namespace NexusEditor
