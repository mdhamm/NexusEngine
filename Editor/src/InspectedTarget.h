#pragma once

#include <QString>

#include <cstdint>
#include <variant>

namespace NexusEditor
{
    struct EntityInspectedTarget
    {
        std::uint64_t m_entityId = 0;

        bool operator==(const EntityInspectedTarget& other) const = default;
    };

    struct AssetInspectedTarget
    {
        QString m_assetPath;

        bool operator==(const AssetInspectedTarget& other) const = default;
    };

    struct InspectedTarget
    {
        using Value = std::variant<std::monostate, EntityInspectedTarget, AssetInspectedTarget>;

        Value m_value;
    };
} // namespace NexusEditor
