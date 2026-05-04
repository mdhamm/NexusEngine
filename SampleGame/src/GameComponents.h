#pragma once

#include <MetadataRegistry.h>

namespace SampleGame
{
    struct RotationSpeed
    {
        float m_x = 0.0f;
        float m_y = 0.0f;
        float m_z = 0.0f;
    };
} // namespace SampleGame

namespace NexusEngine
{
    template<>
    struct ComponentMeta<SampleGame::RotationSpeed>
    {
        static void Register(flecs::world& world, MetadataRegistry& registry)
        {
            world.component<SampleGame::RotationSpeed>()
                .member<float>("x")
                .member<float>("y")
                .member<float>("z");

            registry.component<SampleGame::RotationSpeed>("RotationSpeed")
                .field("x", &SampleGame::RotationSpeed::m_x)
                .field("y", &SampleGame::RotationSpeed::m_y)
                .field("z", &SampleGame::RotationSpeed::m_z);
        }
    };
} // namespace NexusEngine
