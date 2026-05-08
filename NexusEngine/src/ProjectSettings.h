#pragma once

#include <string>
#include <filesystem>

struct ProjectSettings
{
    std::string m_name;
    std::filesystem::path m_rootPath;
};