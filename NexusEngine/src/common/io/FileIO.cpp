#include "FileIO.h"

#include <fstream>

namespace NexusEngine::IO
{
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
} // namespace NexusEngine::IO
