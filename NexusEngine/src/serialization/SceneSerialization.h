#pragma once

#include "Scene.h"

#include <filesystem>
#include <string>
#include <string_view>

namespace NexusEngine
{
    class ISerializeWriter;

    void SerializeScene(const Scene& scene, ISerializeWriter& writer);

    /// <summary>
    /// Saves a scene to a scene asset file.
    /// </summary>
    /// <param name="scene">Scene to save.</param>
    /// <param name="filePath">Destination scene file path.</param>
    /// <returns>True if the scene file was written; otherwise false.</returns>
    bool SaveSceneToFile(const Scene& scene, const std::filesystem::path& filePath);

    /// <summary>
    /// Loads a scene from a scene asset file.
    /// </summary>
    /// <param name="scene">Scene to populate.</param>
    /// <param name="filePath">Source scene file path.</param>
    /// <returns>True if the scene file was loaded; otherwise false.</returns>
    bool LoadSceneFromFile(Scene& scene, const std::filesystem::path& filePath);

} // namespace NexusEngine
