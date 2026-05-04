#include "FileIO.h"

#include "serialization/BinarySerializer.h"
#include "serialization/JsonSerializer.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <unordered_map>

namespace NexusEngine::IO
{
    namespace
    {
        using json = nlohmann::json;

        bool WriteJsonFile(const std::filesystem::path& filePath, const StructuredWriteMethod& writeMethod)
        {
            json document;
            JsonSerializeWriter writer(document);
            writer.BeginObject();
            if (!writeMethod(writer))
            {
                return false;
            }

            const std::string text = document.dump(4) + '\n';
            return WriteFileBytes(
                filePath,
                reinterpret_cast<const std::uint8_t*>(text.data()),
                text.size());
        }

        bool ReadJsonFile(const std::filesystem::path& filePath, const StructuredReadMethod& readMethod)
        {
            const std::optional<std::vector<std::uint8_t>> bytes = ReadFileBytes(filePath);
            if (!bytes)
            {
                return false;
            }

            json document;
            try
            {
                document = json::parse(bytes->begin(), bytes->end());
            }
            catch (...)
            {
                return false;
            }

            if (!document.is_object())
            {
                return false;
            }

            JsonSerializeReader reader(document);
            return readMethod(reader);
        }

        bool WriteBinaryFile(const std::filesystem::path& filePath, const StructuredWriteMethod& writeMethod)
        {
            std::ostringstream stream(std::ios::binary);
            BinarySerializeWriter writer(stream);
            writer.BeginObject();
            if (!writeMethod(writer))
            {
                return false;
            }

            const std::string bytes = stream.str();
            return WriteFileBytes(
                filePath,
                reinterpret_cast<const std::uint8_t*>(bytes.data()),
                bytes.size());
        }

        bool ReadBinaryFile(const std::filesystem::path& filePath, const StructuredReadMethod& readMethod)
        {
            const std::optional<std::vector<std::uint8_t>> bytes = ReadFileBytes(filePath);
            if (!bytes)
            {
                return false;
            }

            const std::string buffer(bytes->begin(), bytes->end());
            std::istringstream stream(buffer, std::ios::binary);
            BinarySerializeReader reader(stream);
            reader.BeginObject();
            return readMethod(reader);
        }
    }

    std::optional<std::vector<std::uint8_t>> ReadFileBytes(const std::filesystem::path& filePath)
    {
        std::ifstream stream(filePath, std::ios::binary);
        if (!stream)
        {
            return std::nullopt;
        }

        stream.seekg(0, std::ios::end);
        const std::streamoff streamSize = stream.tellg();
        if (streamSize < 0)
        {
            return std::nullopt;
        }

        stream.seekg(0, std::ios::beg);

        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(streamSize));
        if (!bytes.empty())
        {
            stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            if (!stream)
            {
                return std::nullopt;
            }
        }

        return bytes;
    }

    bool WriteFileBytes(const std::filesystem::path& filePath, const std::uint8_t* bytes, std::size_t byteCount)
    {
        std::error_code errorCode;
        if (const std::filesystem::path directoryPath = filePath.parent_path(); !directoryPath.empty())
        {
            std::filesystem::create_directories(directoryPath, errorCode);
            if (errorCode)
            {
                return false;
            }
        }

        std::ofstream stream(filePath, std::ios::binary | std::ios::trunc);
        if (!stream)
        {
            return false;
        }

        if (bytes && byteCount > 0)
        {
            stream.write(reinterpret_cast<const char*>(bytes), static_cast<std::streamsize>(byteCount));
        }

        return static_cast<bool>(stream);
    }

    bool WriteFileBytes(const std::filesystem::path& filePath, const std::vector<std::uint8_t>& bytes)
    {
        return WriteFileBytes(filePath, bytes.data(), bytes.size());
    }

    bool WriteStructuredFile(const std::filesystem::path& filePath, FileFormat fileFormat, const StructuredWriteMethod& writeMethod)
    {
        if (!writeMethod)
        {
            return false;
        }

        static const std::unordered_map<FileFormat, bool(*)(const std::filesystem::path&, const StructuredWriteMethod&)> handlers =
        {
            { FileFormat::Json, &WriteJsonFile },
            { FileFormat::Binary, &WriteBinaryFile },
        };

        const auto it = handlers.find(fileFormat);
        if (it == handlers.end())
        {
            return false;
        }

        return it->second(filePath, writeMethod);
    }

    bool ReadStructuredFile(const std::filesystem::path& filePath, FileFormat fileFormat, const StructuredReadMethod& readMethod)
    {
        if (!readMethod)
        {
            return false;
        }

        static const std::unordered_map<FileFormat, bool(*)(const std::filesystem::path&, const StructuredReadMethod&)> handlers =
        {
            { FileFormat::Json, &ReadJsonFile },
            { FileFormat::Binary, &ReadBinaryFile },
        };

        const auto it = handlers.find(fileFormat);
        if (it == handlers.end())
        {
            return false;
        }

        return it->second(filePath, readMethod);
    }
} // namespace NexusEngine::IO
