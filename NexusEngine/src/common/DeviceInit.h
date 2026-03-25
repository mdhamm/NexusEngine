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
        int   width = 1280;
        int   height = 720;
        void* hWnd = nullptr;   // HWND on Win32; void* in headers
    };

    struct GfxContext
    {
        Diligent::RefCntAutoPtr<Diligent::IRenderDevice>  device;
        Diligent::RefCntAutoPtr<Diligent::IDeviceContext> ctx;
        Diligent::RefCntAutoPtr<Diligent::ISwapChain>     swapchain;
        Diligent::RefCntAutoPtr<Diligent::ITexture>       depthTex;
        Diligent::RefCntAutoPtr<Diligent::ITextureView>   depthDSV;
    };

    bool CreateDeviceAndSwapchain(const NativeWindow& win, GfxContext& out);
    void BeginFrame(GfxContext& gfx, float r, float g, float b, float a);
    void EndFrame(GfxContext& gfx);
} // namespace NexusEngine
