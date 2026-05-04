#pragma once

#include "Scene.h"
#include "serialization/ISerializer.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <vector>

namespace NexusEngine
{
    using SerializeSceneMethod = std::function<bool(const Scene& scene, ISerializeWriter& writer)>;
    using DeserializeSceneMethod = std::function<bool(Scene& scene, ISerializeReader& reader)>;
}

namespace NexusEngine::IO
{
    using CreateSceneWriterMethod = std::function<bool(std::vector<std::uint8_t>& bytes, const Scene& scene, const SerializeSceneMethod& serializeMethod)>;
    using CreateSceneReaderMethod = std::function<bool(const std::vector<std::uint8_t>& bytes, Scene& scene, const DeserializeSceneMethod& deserializeMethod)>;

    /// <summary>
    /// Saves a scene to a file using caller-provided serialization methods.
    /// </summary>
    /// <param name="scene">Scene to save.</param>
    /// <param name="filePath">Destination scene file path.</param>
    /// <param name="serializeMethod">Serialization method that writes scene data to the provided writer.</param>
    /// <param name="createWriter">Factory that serializes the scene into a byte buffer.</param>
    /// <returns>True if the scene file was written; otherwise false.</returns>
    bool SaveSceneToFile(
        const Scene& scene,
        const std::filesystem::path& filePath,
        const SerializeSceneMethod& serializeMethod,
        const CreateSceneWriterMethod& createWriter);

    /// <summary>
    /// Loads a scene from a file using caller-provided deserialization methods.
    /// </summary>
    /// <param name="scene">Scene to populate.</param>
    /// <param name="filePath">Source scene file path.</param>
    /// <param name="deserializeMethod">Deserialization method that reads scene data from the provided reader.</param>
    /// <param name="createReader">Factory that deserializes the scene from a byte buffer.</param>
    /// <returns>True if the scene file was loaded; otherwise false.</returns>
    bool LoadSceneFromFile(
        Scene& scene,
        const std::filesystem::path& filePath,
        const DeserializeSceneMethod& deserializeMethod,
        const CreateSceneReaderMethod& createReader);
} // namespace NexusEngine::IO
