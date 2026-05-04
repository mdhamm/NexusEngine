#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <vector>

namespace NexusEngine::IO
{
    using SerializeToBytesMethod = std::function<bool(std::vector<std::uint8_t>& bytes)>;
    using DeserializeFromBytesMethod = std::function<bool(const std::vector<std::uint8_t>& bytes)>;

    /// <summary>
    /// Reads an entire file as raw bytes.
    /// </summary>
    /// <param name="filePath">Path to the file.</param>
    /// <returns>The file bytes when successful; otherwise no value.</returns>
    std::optional<std::vector<std::uint8_t>> ReadFileBytes(const std::filesystem::path& filePath);

    /// <summary>
    /// Writes raw bytes to a file, creating parent directories when needed.
    /// </summary>
    /// <param name="filePath">Destination file path.</param>
    /// <param name="bytes">Pointer to the source byte buffer.</param>
    /// <param name="byteCount">Number of bytes to write.</param>
    /// <returns>True if the file was written; otherwise false.</returns>
    bool WriteFileBytes(const std::filesystem::path& filePath, const std::uint8_t* bytes, std::size_t byteCount);

    /// <summary>
    /// Writes raw bytes to a file, creating parent directories when needed.
    /// </summary>
    /// <param name="filePath">Destination file path.</param>
    /// <param name="bytes">Source byte buffer.</param>
    /// <returns>True if the file was written; otherwise false.</returns>
    bool WriteFileBytes(const std::filesystem::path& filePath, const std::vector<std::uint8_t>& bytes);

    /// <summary>
    /// Serializes data into bytes and writes the result to a file.
    /// </summary>
    /// <param name="filePath">Destination file path.</param>
    /// <param name="serializeMethod">Callback that fills the output byte buffer.</param>
    /// <returns>True if serialization and file writing succeeded; otherwise false.</returns>
    bool WriteSerializedFile(const std::filesystem::path& filePath, const SerializeToBytesMethod& serializeMethod);

    /// <summary>
    /// Reads file bytes and passes them to a caller-provided deserializer.
    /// </summary>
    /// <param name="filePath">Source file path.</param>
    /// <param name="deserializeMethod">Callback that consumes the file bytes.</param>
    /// <returns>True if file reading and deserialization succeeded; otherwise false.</returns>
    bool ReadSerializedFile(const std::filesystem::path& filePath, const DeserializeFromBytesMethod& deserializeMethod);
} // namespace NexusEngine::IO
