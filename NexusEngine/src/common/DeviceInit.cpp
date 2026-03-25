#include "DeviceInit.h"

#include <DiligentCore/Graphics/GraphicsEngine/interface/SwapChain.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <Windows.h>
#include <DiligentCore/Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h>
#include <DiligentCore/Platforms/Win32/interface/Win32NativeWindow.h>
#elif defined(__EMSCRIPTEN__)
#include <DiligentCore/Graphics/GraphicsEngine/interface/EngineFactoryWebGPU.h>
#include <DiligentTools/Platform/Web/interface/WebGPUNativeWindow.h>
#endif

using namespace Diligent;

namespace NexusEngine
{
    bool CreateDeviceAndSwapchain(const NativeWindow& win, GfxContext& out)
    {
#if defined(_WIN32)
        EngineD3D12CreateInfo engCI{};
        RefCntAutoPtr<IEngineFactoryD3D12> factory{ GetEngineFactoryD3D12() };
        if (!factory) return false;

        SwapChainDesc scDesc{};
        scDesc.Width = static_cast<Uint32>(win.width);
        scDesc.Height = static_cast<Uint32>(win.height);
        scDesc.ColorBufferFormat = TEX_FORMAT_RGBA8_UNORM_SRGB;

        Win32NativeWindow wnd{};
        wnd.hWnd = static_cast<HWND>(win.hWnd);

        RefCntAutoPtr<IRenderDevice>  device;
        RefCntAutoPtr<IDeviceContext> ctx;
        RefCntAutoPtr<ISwapChain>     sc;

        factory->CreateDeviceAndContextsD3D12(engCI, &device, &ctx);
        factory->CreateSwapChainD3D12(device, ctx, scDesc, FullScreenModeDesc{}, wnd, &sc);

        out.device = device;
        out.ctx = ctx;
        out.swapchain = sc;

        {
            TextureDesc depthDesc{};
            depthDesc.Name = "ImGui.Depth";
            depthDesc.Type = RESOURCE_DIM_TEX_2D;
            depthDesc.Width = scDesc.Width;
            depthDesc.Height = scDesc.Height;
            depthDesc.MipLevels = 1;
            depthDesc.Format = scDesc.DepthBufferFormat;
            depthDesc.BindFlags = BIND_DEPTH_STENCIL;

            device->CreateTexture(depthDesc, nullptr, &out.depthTex);
            out.depthDSV = out.depthTex->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
        }

        return out.swapchain != nullptr;

#elif defined(__EMSCRIPTEN__)
        EngineWebGPUCreateInfo engCI{};
        RefCntAutoPtr<IEngineFactoryWebGPU> factory{ GetEngineFactoryWebGPU() };
        if (!factory) return false;

        SwapChainDesc scDesc{};
        scDesc.Width = static_cast<Uint32>(win.width);
        scDesc.Height = static_cast<Uint32>(win.height);
        scDesc.ColorBufferFormat = TEX_FORMAT_RGBA8_UNORM_SRGB;

        WebGPUNativeWindow wnd{}; // default canvas

        RefCntAutoPtr<IRenderDevice>  device;
        RefCntAutoPtr<IDeviceContext> ctx;
        RefCntAutoPtr<ISwapChain>     sc;

        factory->CreateDeviceAndContextsWebGPU(engCI, &device, &ctx);
        factory->CreateSwapChainWebGPU(device, ctx, scDesc, wnd, &sc);

        out.device = device;
        out.ctx = ctx;
        out.swapchain = sc;
        return out.swapchain != nullptr;

#else
        (void)win; (void)out;
        return false;
#endif
    }

    void BeginFrame(GfxContext& gfx, float r, float g, float b, float a)
    {
        if (!gfx.swapchain || !gfx.ctx) return;

        auto* pRTV = gfx.swapchain->GetCurrentBackBufferRTV();
        Diligent::ITextureView* pDSV = gfx.depthDSV.RawPtr();
        const float clear[4] = { r, g, b, a };

        gfx.ctx->SetRenderTargets(1, &pRTV, pDSV,
            Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        gfx.ctx->ClearRenderTarget(pRTV, clear,
            Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        if (pDSV)
            gfx.ctx->ClearDepthStencil(pDSV,
                Diligent::CLEAR_DEPTH_FLAG, 1.0f, 0,
                Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }

    void EndFrame(GfxContext& gfx)
    {
        if (gfx.swapchain)
        {
            gfx.swapchain->Present();
        }
    }
} // namespace NexusEngine
