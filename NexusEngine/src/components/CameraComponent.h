#pragma once

#include "MetadataRegistry.h"
#include "RenderTextureComponent.h"

namespace NexusEngine
{
    struct CameraComponent;

    // Camera component describing where a scene should render.
    struct CameraComponent
    {
        // Output target used by the camera.
        enum class Target
        {
            // Render to the main swap chain back buffer.
            SwapChain,

            // Render into a texture target.
            RenderTexture
        };

        // Output mode for the camera.
        Target m_target = Target::SwapChain;

        // Render texture used when targeting an off-screen surface.
        RenderTextureComponent* m_renderTexture = nullptr;

        // Camera sort priority. Higher values render later.
        int m_priority = 0;     // higher = rendered later (on top)

    };

    template<>
    struct ComponentMeta<CameraComponent>
    {
        static void Register(flecs::world& world, MetadataRegistry& registry)
        {
            world.component<CameraComponent>()
                .member<CameraComponent::Target>("target")
                .member<int>("priority");

            registry.component<CameraComponent>("CameraComponent")
                .field("Target", &CameraComponent::m_target)
                    .enumValue("SwapChain", static_cast<int>(CameraComponent::Target::SwapChain))
                    .enumValue("RenderTexture", static_cast<int>(CameraComponent::Target::RenderTexture))
                .field("Priority", &CameraComponent::m_priority);
        }
    };
} // namespace NexusEngine
