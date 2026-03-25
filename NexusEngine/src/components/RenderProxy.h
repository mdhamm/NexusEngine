#pragma once
#include <functional>
#include <DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h>

namespace NexusEngine
{
    // Component for entities that wish to render via a callback.
    struct RenderProxy
    {
        std::function<void(Diligent::IDeviceContext*)> draw;
    };
} // namespace NexusEngine