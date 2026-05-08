#pragma once

#include <string>
#include <filesystem>

namespace NexusEngine
{
    struct ProjectSettings
    {
        std::string m_name;
        std::filesystem::path m_rootPath;
    };
} // namespace NexusEngine