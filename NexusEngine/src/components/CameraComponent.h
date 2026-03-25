#pragma once
#include <DiligentCore/Graphics/GraphicsEngine/interface/TextureView.h>
#include <flecs.h>

namespace NexusEngine
{
    struct CameraComponent
    {
        Diligent::ITextureView* targetRTV = nullptr;
        Diligent::ITextureView* targetDSV = nullptr;
        Diligent::IDeviceContext* deviceCtx = nullptr;

        flecs::entity canvas; // optional overlay UI canvas
        int priority = 0;     // higher = rendered later (on top)
    };
} // namespace NexusEngine
