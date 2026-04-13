#pragma once

#include "RenderTextureComponent.h"

namespace NexusEngine
{
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
} // namespace NexusEngine
