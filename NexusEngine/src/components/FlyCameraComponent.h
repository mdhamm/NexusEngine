#pragma once

#include "MetadataRegistry.h"

namespace NexusEngine
{
    struct FlyCameraComponent;

    // Fly camera controller component that allows controlling a camera with free movement and rotation, typically used for editor cameras.
    struct FlyCameraComponent
    {
        float m_moveSpeed = 12.0f;
        float m_lookSensitivity = 0.003f;
        float m_yaw = 0.0f;
        float m_pitch = 0.0f;
        bool m_isRotationInitialized = false;

    };

    template<>
    struct ComponentMeta<FlyCameraComponent>
    {
        static void Register(flecs::world& world, MetadataRegistry& registry)
        {
            world.component<FlyCameraComponent>()
                .member<float>("moveSpeed")
                .member<float>("lookSensitivity")
                .member<float>("yaw")
                .member<float>("pitch")
                .member<bool>("isRotationInitialized");

            registry.component<FlyCameraComponent>("FlyCameraComponent")
                .field("Move Speed", &FlyCameraComponent::m_moveSpeed)
                .field("Look Sensitivity", &FlyCameraComponent::m_lookSensitivity)
                .field("Yaw", &FlyCameraComponent::m_yaw)
                .field("Pitch", &FlyCameraComponent::m_pitch)
                .field("Rotation Initialized", &FlyCameraComponent::m_isRotationInitialized);
        }
    };
} // namespace NexusEngine