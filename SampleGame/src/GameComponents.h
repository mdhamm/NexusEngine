#pragma once

#include <reflection/EntityReflection.h>

namespace SampleGame
{
    struct RotationSpeed
    {
        float m_x = 0.0f;
        float m_y = 0.0f;
        float m_z = 0.0f;
    };

    struct World
    {
    };

    struct WorldInitialized
    {
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

    template<>
    struct ComponentMeta<SampleGame::World>
    {
        static void Register(flecs::world& world, MetadataRegistry& registry)
        {
            world.component<SampleGame::World>();
            registry.component<SampleGame::World>("World");
        }
    };

    template<>
    struct ComponentMeta<SampleGame::WorldInitialized>
    {
        static void Register(flecs::world& world, MetadataRegistry& registry)
        {
            world.component<SampleGame::WorldInitialized>();
            registry.component<SampleGame::WorldInitialized>("WorldInitialized");
        }
    };
} // namespace NexusEngine
