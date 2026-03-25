#pragma once
#include <functional>

namespace NexusEngine
{
    struct CanvasComponent
    {
        // The function that draws the overlay for this canvas
        std::function<void()> uiLayerFn;
    };
} // namespace NexusEngine
