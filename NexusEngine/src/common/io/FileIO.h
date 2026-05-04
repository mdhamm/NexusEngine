#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace NexusEngine::IO
{
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
} // namespace NexusEngine::IO
