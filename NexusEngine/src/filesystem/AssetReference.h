#pragma once

#include <string>

namespace NexusEngine::IO
{
    struct AssetReference
    {
        std::string m_guid;

        bool IsEmpty() const
        {
            return m_guid.empty();
        }

        std::string GetGuid() const
        {
            return m_guid;
        }
    };
} // namespace NexusEngine::IO
