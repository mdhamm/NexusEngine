#pragma once

#include <cstddef>
#include <concepts>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <type_traits>
#include <vector>

namespace NexusEngine
{
    class ISerializeReader;
    class ISerializeWriter;
}

namespace NexusEngine::IO
{
    enum class FileFormat
    {
        Json,
        Binary,
    };

    using StructuredWriteMethod = std::function<bool(ISerializeWriter& writer)>;
    using StructuredReadMethod = std::function<bool(ISerializeReader& reader)>;

    template <typename T>
    concept SerializableToFile = requires(const T& value, ISerializeWriter& writer)
    {
        { Serialize(value, writer) } -> std::same_as<void>;
    };

    template <typename T>
    concept DeserializableFromFile = requires(T& value, ISerializeReader& reader)
    {
        { Deserialize(value, reader) } -> std::convertible_to<bool>;
    };

    template <typename T>
    concept FileSerializable = SerializableToFile<T> && DeserializableFromFile<T>;

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
    /// Writes structured data to a file using the requested file format.
    /// </summary>
    /// <param name="filePath">Destination file path.</param>
    /// <param name="fileFormat">Structured file format to write.</param>
    /// <param name="writeMethod">Method that writes structured data to the provided serializer.</param>
    /// <returns>True if serialization and file writing succeeded; otherwise false.</returns>
    bool WriteStructuredFile(const std::filesystem::path& filePath, FileFormat fileFormat, const StructuredWriteMethod& writeMethod);

    /// <summary>
    /// Reads structured data from a file using the requested file format.
    /// </summary>
    /// <param name="filePath">Source file path.</param>
    /// <param name="fileFormat">Structured file format to read.</param>
    /// <param name="readMethod">Method that reads structured data from the provided serializer.</param>
    /// <returns>True if file reading and deserialization succeeded; otherwise false.</returns>
    bool ReadStructuredFile(const std::filesystem::path& filePath, FileFormat fileFormat, const StructuredReadMethod& readMethod);

    /// <summary>
    /// Saves a serializable value to a file using the requested file format.
    /// </summary>
    /// <typeparam name="T">Serializable value type.</typeparam>
    /// <param name="value">Value to serialize.</param>
    /// <param name="filePath">Destination file path.</param>
    /// <param name="fileFormat">Structured file format to write.</param>
    /// <returns>True if the value was serialized and written; otherwise false.</returns>
    template <SerializableToFile T>
    bool SaveToFile(const T& value, const std::filesystem::path& filePath, FileFormat fileFormat = FileFormat::Json)
    {
        return WriteStructuredFile(
            filePath,
            fileFormat,
            [&](ISerializeWriter& writer)
            {
                Serialize(value, writer);
                return true;
            });
    }

    /// <summary>
    /// Loads a serializable value from a file using the requested file format.
    /// </summary>
    /// <typeparam name="T">Deserializable value type.</typeparam>
    /// <param name="value">Value to populate.</param>
    /// <param name="filePath">Source file path.</param>
    /// <param name="fileFormat">Structured file format to read.</param>
    /// <returns>True if the file was read and the value was deserialized; otherwise false.</returns>
    template <DeserializableFromFile T>
    bool LoadFromFile(T& value, const std::filesystem::path& filePath, FileFormat fileFormat = FileFormat::Json)
    {
        return ReadStructuredFile(
            filePath,
            fileFormat,
            [&](ISerializeReader& reader)
            {
                return Deserialize(value, reader);
            });
    }
} // namespace NexusEngine::IO
