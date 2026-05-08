#pragma once

#include "filesystem/AssetReference.h"

#include <string>

namespace NexusEngine
{
    struct ProjectSettings
    {
        std::string m_name;
        IO::AssetReference m_defaultScene;
    };
} // namespace NexusEngine