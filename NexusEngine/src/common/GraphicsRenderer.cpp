#include "GraphicsRenderer.h"

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
#include <DiligentCore/Graphics/GraphicsEngineWebGPU/interface/EngineFactoryWebGPU.h>
#endif

using namespace Diligent;

namespace NexusEngine
{
    bool GraphicsRenderer::CreateDeviceAndSwapchain(const NativeWindow& win)
    {
#if defined(_WIN32)
        EngineD3D12CreateInfo engCI{};
        RefCntAutoPtr<IEngineFactoryD3D12> factory{ GetEngineFactoryD3D12() };
        if (!factory)
        {
            return false;
        }

        SwapChainDesc scDesc{};
        scDesc.Width = static_cast<Uint32>(win.m_width);
        scDesc.Height = static_cast<Uint32>(win.m_height);
        scDesc.ColorBufferFormat = TEX_FORMAT_RGBA8_UNORM_SRGB;
        scDesc.DepthBufferFormat = TEX_FORMAT_D32_FLOAT;

        Diligent::Win32NativeWindow wnd{};
        wnd.hWnd = win.m_hWnd;

        RefCntAutoPtr<IRenderDevice>  device;
        RefCntAutoPtr<IDeviceContext> ctx;
        RefCntAutoPtr<ISwapChain>     sc;

        factory->CreateDeviceAndContextsD3D12(engCI, &device, &ctx);
        factory->CreateSwapChainD3D12(device, ctx, scDesc, FullScreenModeDesc{}, wnd, &sc);

        m_gfx.m_device = device;
        m_gfx.m_ctx = ctx;
        m_gfx.m_swapchain = sc;

        {
            TextureDesc depthDesc{};
            depthDesc.Name = "ImGui.Depth";
            depthDesc.Type = RESOURCE_DIM_TEX_2D;
            depthDesc.Width = scDesc.Width;
            depthDesc.Height = scDesc.Height;
            depthDesc.MipLevels = 1;
            depthDesc.Format = scDesc.DepthBufferFormat;
            depthDesc.BindFlags = BIND_DEPTH_STENCIL;

            device->CreateTexture(depthDesc, nullptr, &m_gfx.m_depthTex);
            m_gfx.m_depthDSV = m_gfx.m_depthTex->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
        }

        return m_gfx.m_swapchain != nullptr;

#elif defined(__EMSCRIPTEN__)
        EngineWebGPUCreateInfo engCI{};

        RefCntAutoPtr<IEngineFactoryWebGPU> factory{GetEngineFactoryWebGPU()};
        if (!factory)
        {
            return false;
        }

        SwapChainDesc scDesc{};
        scDesc.Width = static_cast<Uint32>(win.m_width);
        scDesc.Height = static_cast<Uint32>(win.m_height);
        scDesc.ColorBufferFormat = TEX_FORMAT_RGBA8_UNORM;
        scDesc.DepthBufferFormat = TEX_FORMAT_D32_FLOAT;

        RefCntAutoPtr<IRenderDevice> device;
        RefCntAutoPtr<IDeviceContext> ctx;
        RefCntAutoPtr<ISwapChain> sc;

        IDeviceContext* ppContexts[] = {nullptr};

        factory->CreateDeviceAndContextsWebGPU(engCI, &device, ppContexts);
        ctx = ppContexts[0];
        if (!device || !ctx)
        {
            return false;
        }

        factory->CreateSwapChainWebGPU(device, ctx, scDesc, Diligent::NativeWindow{win.m_canvasId}, &sc);

        m_gfx.m_device = device;
        m_gfx.m_ctx = ctx;
        m_gfx.m_swapchain = sc;

        return m_gfx.m_device != nullptr && m_gfx.m_ctx != nullptr && m_gfx.m_swapchain != nullptr;

#else
        (void)win;
        return false;
#endif
    }

    void GraphicsRenderer::BeginFrame(float r, float g, float b, float a)
    {
        if (!m_gfx.m_swapchain || !m_gfx.m_ctx)
        {
            return;
        }

        Diligent::ITextureView* pRTV = m_gfx.m_swapchain->GetCurrentBackBufferRTV();
        Diligent::ITextureView* pDSV = m_gfx.m_swapchain->GetDepthBufferDSV();

        const float clear[4] = { r, g, b, a };
        m_gfx.m_ctx->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_gfx.m_ctx->ClearRenderTarget(pRTV, clear, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        if (pDSV)
        {
            m_gfx.m_ctx->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.0f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        }
    }

    void GraphicsRenderer::EndFrame()
    {
        if (m_gfx.m_swapchain)
        {
            m_gfx.m_swapchain->Present();
        }
    }
} // namespace NexusEngine
