#pragma once

#include <QString>

#include <filesystem/AssetReference.h>

namespace NexusEditor
{
    struct EditorProject
    {
        QString m_name;
        QString m_rootPath;
        NexusEngine::IO::AssetReference m_defaultScene;
        bool m_requiresInitialBuild = false;
    };
} // namespace NexusEditor
