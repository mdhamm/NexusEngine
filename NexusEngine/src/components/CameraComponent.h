#pragma once

#include "RenderTextureComponent.h"

namespace NexusEngine
{
    struct CameraComponent
    {
        enum class Target
        {
            SwapChain,
            RenderTexture
        };

        Target m_target = Target::SwapChain;
        RenderTextureComponent* m_renderTexture = nullptr;

        int m_priority = 0;     // higher = rendered later (on top)
    };
} // namespace NexusEngine
