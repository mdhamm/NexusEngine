#include "AssetReferenceRegistry.h"

#include "AssetGuid.h"
#include "FileIO.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

namespace NexusEngine::IO
{
    namespace
    {
        using json = nlohmann::json;

        std::string Trim(std::string_view text)
        {
            size_t start = 0;
            size_t end = text.size();
            while (start < end && std::isspace(static_cast<unsigned char>(text[start])) != 0)
            {
                ++start;
            }

            while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0)
            {
                --end;
            }

            return std::string(text.substr(start, end - start));
        }

        std::string ToLowerAscii(std::string_view text)
        {
            std::string lowered(text);
            std::transform(
                lowered.begin(),
                lowered.end(),
                lowered.begin(),
                [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
            return lowered;
        }

        std::string NormalizePath(const std::string& path)
        {
            if (path.empty())
            {
                return {};
            }

            return std::filesystem::path(path).lexically_normal().string();
        }

        std::string ToStoredAssetPath(const std::string& projectRootPath, const std::string& assetPath)
        {
            const std::filesystem::path rootPath = std::filesystem::path(projectRootPath).lexically_normal();
            const std::filesystem::path normalizedAssetPath = std::filesystem::path(assetPath).lexically_normal();
            const std::filesystem::path relativePath = normalizedAssetPath.lexically_relative(rootPath);
            const std::string relativePathText = relativePath.generic_string();
            return relativePath.empty() || relativePathText.rfind("..", 0) == 0
                ? normalizedAssetPath.string()
                : relativePath.string();
        }

        std::string ToAbsoluteAssetPath(const std::string& projectRootPath, const std::string& storedPath)
        {
            if (storedPath.empty())
            {
                return {};
            }

            const std::filesystem::path path(storedPath);
            return NormalizePath(path.is_absolute() ? path.string() : (std::filesystem::path(projectRootPath) / path).string());
        }

        std::string GetAssetReferenceFilePath(const std::string& projectRootPath, const std::string& guid)
        {
            return NormalizePath((std::filesystem::path(GetAssetReferenceDirectoryPath(projectRootPath)) / (guid + ".json")).string());
        }

        std::string ReadReferencedPath(const std::string& projectRootPath, const std::string& guid)
        {
            const std::optional<std::vector<std::uint8_t>> fileBytes = ReadFileBytes(GetAssetReferenceFilePath(projectRootPath, guid));
            if (!fileBytes)
            {
                return {};
            }

            try
            {
                const json document = json::parse(fileBytes->begin(), fileBytes->end());
                return document.is_object()
                    ? NormalizePath(ToAbsoluteAssetPath(projectRootPath, Trim(document.value("path", std::string{}))))
                    : std::string{};
            }
            catch (...)
            {
                return {};
            }
        }

        bool WriteReferenceFile(const std::string& projectRootPath, const std::string& guid, const std::string& assetPath)
        {
            if (projectRootPath.empty() || guid.empty() || assetPath.empty())
            {
                return false;
            }

            json referenceObject;
            referenceObject["guid"] = guid;
            referenceObject["path"] = ToStoredAssetPath(projectRootPath, assetPath);
            const std::string text = referenceObject.dump(4) + '\n';
            return WriteFileBytes(
                GetAssetReferenceFilePath(projectRootPath, guid),
                reinterpret_cast<const std::uint8_t*>(text.data()),
                text.size());
        }
    }

    AssetReference AssetReferenceFromPath(const std::string& path)
    {
        const std::string cleanPath = NormalizePath(path);
        if (cleanPath.empty())
        {
            return {};
        }

        const std::string guid = ReadAssetReferenceGuidForPath(cleanPath);
        return AssetReference{ guid };
    }

    AssetReference CreateAssetReference(const std::string& path)
    {
        const std::string cleanPath = NormalizePath(path);
        if (cleanPath.empty())
        {
            return {};
        }

        const std::string projectRootPath = FindProjectRootPath(cleanPath);
        std::string guid = ReadAssetReferenceGuidForPath(cleanPath);
        if (guid.empty())
        {
            guid = EnsureAssetReferenceGuid(cleanPath);
        }

        if (guid.empty())
        {
            return {};
        }

        return WriteReferenceFile(projectRootPath, guid, cleanPath)
            ? AssetReference{ guid }
            : AssetReference{};
    }

    bool NotifyAssetPathChanged(const std::string& oldPath, const std::string& newPath)
    {
        const std::string cleanOldPath = NormalizePath(oldPath);
        const std::string cleanNewPath = NormalizePath(newPath);
        const std::string projectRootPath = FindProjectRootPath(cleanNewPath.empty() ? cleanOldPath : cleanNewPath);
        if (projectRootPath.empty())
        {
            return false;
        }

        const std::filesystem::path referenceDirectoryPath(GetAssetReferenceDirectoryPath(projectRootPath));
        std::error_code errorCode;
        if (!std::filesystem::exists(referenceDirectoryPath, errorCode) || errorCode)
        {
            return false;
        }

        for (const auto& entry : std::filesystem::directory_iterator(referenceDirectoryPath, errorCode))
        {
            if (errorCode)
            {
                return false;
            }

            if (!entry.is_regular_file(errorCode) || errorCode || entry.path().extension() != ".json")
            {
                continue;
            }

            const std::optional<std::vector<std::uint8_t>> fileBytes = ReadFileBytes(entry.path());
            if (!fileBytes)
            {
                continue;
            }

            json document;
            try
            {
                document = json::parse(fileBytes->begin(), fileBytes->end());
            }
            catch (...)
            {
                continue;
            }

            if (!document.is_object())
            {
                continue;
            }

            const std::string storedPath = NormalizePath(ToAbsoluteAssetPath(projectRootPath, Trim(document.value("path", std::string{}))));
            if (ToLowerAscii(storedPath) != ToLowerAscii(cleanOldPath))
            {
                continue;
            }

            const std::string guid = Trim(document.value("guid", std::string{}));
            return WriteReferenceFile(projectRootPath, guid, cleanNewPath);
        }

        return false;
    }

    std::string GetAssetReferencePath(const AssetReference& reference, const std::string& projectRootPath)
    {
        if (reference.m_guid.empty())
        {
            return {};
        }

        return ReadReferencedPath(NormalizePath(projectRootPath), reference.m_guid);
    }

    bool ResolveAssetReferencePath(const AssetReference& reference, const std::string& projectRootPath, std::string& resolvedPath)
    {
        resolvedPath = GetAssetReferencePath(reference, projectRootPath);
        if (resolvedPath.empty())
        {
            return false;
        }

        const std::filesystem::path referencedFilePath(resolvedPath);
        return std::filesystem::exists(referencedFilePath) || std::filesystem::exists(referencedFilePath.parent_path());
    }

    std::string FindProjectRootPath(const std::string& path)
    {
        std::filesystem::path currentPath = std::filesystem::path(path);
        if (!std::filesystem::is_directory(currentPath))
        {
            currentPath = currentPath.parent_path();
        }

        while (!currentPath.empty())
        {
            if (std::filesystem::exists(currentPath / "NexusProject.json"))
            {
                return NormalizePath(currentPath.string());
            }

            const std::filesystem::path parentPath = currentPath.parent_path();
            if (parentPath == currentPath)
            {
                break;
            }

            currentPath = parentPath;
        }

        return NormalizePath(std::filesystem::is_directory(std::filesystem::path(path))
            ? path
            : std::filesystem::path(path).parent_path().string());
    }

    std::string GetAssetReferenceDirectoryPath(const std::string& projectRootPath)
    {
        return NormalizePath((std::filesystem::path(projectRootPath) / ".nexus" / "assetrefs").string());
    }

    bool IsSceneFilePath(const std::string& path)
    {
        return ToLowerAscii(std::filesystem::path(path).extension().string()) == ".nscene";
    }

    std::string ReadAssetReferenceGuidForPath(const std::string& path)
    {
        if (IsSceneFilePath(path))
        {
            const std::optional<std::vector<std::uint8_t>> fileBytes = ReadFileBytes(path);
            if (!fileBytes)
            {
                return {};
            }

            try
            {
                const json document = json::parse(fileBytes->begin(), fileBytes->end());
                return document.is_object() ? Trim(document.value("guid", std::string{})) : std::string{};
            }
            catch (...)
            {
                return {};
            }
        }

        return {};
    }

    std::string EnsureAssetReferenceGuid(const std::string& path)
    {
        if (!IsSceneFilePath(path))
        {
            return CreateAssetGuid();
        }

        const std::optional<std::vector<std::uint8_t>> fileBytes = ReadFileBytes(path);
        if (!fileBytes)
        {
            return {};
        }

        json document;
        try
        {
            document = json::parse(fileBytes->begin(), fileBytes->end());
        }
        catch (...)
        {
            return {};
        }

        if (!document.is_object())
        {
            return {};
        }

        const std::string existingGuid = Trim(document.value("guid", std::string{}));
        if (!existingGuid.empty())
        {
            return existingGuid;
        }

        const std::string assetGuid = CreateAssetGuid();
        document["guid"] = assetGuid;
        const std::string text = document.dump(4) + '\n';
        return WriteFileBytes(
            path,
            reinterpret_cast<const std::uint8_t*>(text.data()),
            text.size())
            ? assetGuid
            : std::string{};
    }
} // namespace NexusEngine::IO
