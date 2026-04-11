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
    struct NativeWindow
    {
        int   m_width = 1280;
        int   m_height = 720;
        void* m_hWnd = nullptr;   // HWND on Win32; void* in headers
        const char* m_canvasId = "#canvas";
    };

    struct GfxContext
    {
        Diligent::RefCntAutoPtr<Diligent::IRenderDevice>  m_device;
        Diligent::RefCntAutoPtr<Diligent::IDeviceContext> m_ctx;
        Diligent::RefCntAutoPtr<Diligent::ISwapChain>     m_swapchain;
        Diligent::RefCntAutoPtr<Diligent::ITexture>       m_depthTex;
        Diligent::RefCntAutoPtr<Diligent::ITextureView>   m_depthDSV;
    };

    class GraphicsRenderer
    {
    public:
        bool CreateDeviceAndSwapchain(const NativeWindow& win);
        void BeginFrame(float r, float g, float b, float a);
        void EndFrame();

        GfxContext m_gfx;
    };
} // namespace NexusEngine
