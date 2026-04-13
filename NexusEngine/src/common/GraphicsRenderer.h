// DeviceInit.h
#pragma once

// Ref-count smart pointer
#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>

// Pull in the actual interface definitions (avoid forward-decl mismatches)
#include <DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/SwapChain.h>

namespace NexusEngine
{
    // Platform-agnostic window description used during renderer startup.
    struct NativeWindow
    {
        // Initial window width in pixels.
        int   m_width = 1280;

        // Initial window height in pixels.
        int   m_height = 720;

        // Native window handle on desktop platforms.
        void* m_hWnd = nullptr;   // HWND on Win32; void* in headers

        // HTML canvas selector used by web builds.
        const char* m_canvasId = "#canvas";
    };

    // Bundle of core rendering interfaces used by the engine.
    struct GfxContext
    {
        // Render device used to allocate GPU resources.
        Diligent::RefCntAutoPtr<Diligent::IRenderDevice>  m_device;

        // Immediate device context used for rendering commands.
        Diligent::RefCntAutoPtr<Diligent::IDeviceContext> m_ctx;

        // Swap chain presenting frames to the output surface.
        Diligent::RefCntAutoPtr<Diligent::ISwapChain>     m_swapchain;

        // Depth texture owned by the renderer.
        Diligent::RefCntAutoPtr<Diligent::ITexture>       m_depthTex;

        // Depth-stencil view for the depth texture.
        Diligent::RefCntAutoPtr<Diligent::ITextureView>   m_depthDSV;
    };

    // Thin wrapper around device creation and frame presentation.
    class GraphicsRenderer
    {
    public:
        /// <summary>
        /// Creates the rendering device and swap chain for a window.
        /// </summary>
        /// <param name="win">Native window description to bind to.</param>
        /// <returns>True if device creation succeeds; otherwise false.</returns>
        bool CreateDeviceAndSwapchain(const NativeWindow& win);

        /// <summary>
        /// Begins a frame and clears the render targets.
        /// </summary>
        /// <param name="r">Red clear value.</param>
        /// <param name="g">Green clear value.</param>
        /// <param name="b">Blue clear value.</param>
        /// <param name="a">Alpha clear value.</param>
        void BeginFrame(float r, float g, float b, float a);

        /// <summary>
        /// Ends the current frame and presents it.
        /// </summary>
        void EndFrame();

        // Core graphics interfaces used by the rest of the engine.
        GfxContext m_gfx;
    };
} // namespace NexusEngine
