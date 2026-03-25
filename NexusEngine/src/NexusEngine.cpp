#include "NexusEngine.h"
#include "common/DeviceInit.h"
#include "Scene.h"

// Components
#include "components/CameraComponent.h"
#include "components/CanvasComponent.h"
#include "components/RenderProxy.h"

#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <DiligentTools/Imgui/interface/ImGuiImplDiligent.hpp>

#include <chrono>
#include <cmath>
#include <algorithm>

using namespace Diligent;

namespace NexusEngine
{
    // --------------------------------------------------------------
    // Initialization / Shutdown
    // --------------------------------------------------------------

    bool Engine::Initialize(const NativeWindow& win, std::unique_ptr<IGameApp> game)
    {
        assert(game);
        if (!game)
        {
            return false;
        }
        m_game = game.release();
        bool graphicsInitialized = CreateDeviceAndSwapchain(win, gfx_);
        m_initialized = true;
    }

    void Engine::Start()
    {
        assert(m_initialized);
        if (!m_initialized)
        {
            return;
        }

        if (m_game)
        {
            m_game->OnStartup(*this);
        }

        // Game loop
        using clock = std::chrono::high_resolution_clock;
        auto lastTime = clock::now();
        while (!m_shutdown)
        {
            auto now = clock::now();
            std::chrono::duration<float> delta = now - lastTime;
            lastTime = now;
            Tick(delta.count());
        }

        if (m_game)
        {
            m_game->OnShutdown(*this);
        }
    }

    void Engine::Shutdown()
    {
        assert(m_initialized);
        if (!m_initialized)
        {
            return;
        }

        DisableImGui();
        scenes_.clear();
        gfx_.swapchain.Release();
        gfx_.ctx.Release();
        gfx_.device.Release();
    }

    // --------------------------------------------------------------
    // Scene Management
    // --------------------------------------------------------------

    Scene& Engine::CreateScene(const std::string& name)
    {
        auto s = std::make_unique<Scene>(name);
        RegisterSceneComponents(s->m_world);
        Scene& ref = *s;
        scenes_.push_back(std::move(s));
        return ref;
    }

    void Engine::RemoveScene(const std::string& name)
    {
        scenes_.erase(std::remove_if(scenes_.begin(), scenes_.end(),
            [&](auto& sc)
            {
                return sc->m_name == name;
            }),
            scenes_.end());
    }

    Scene* Engine::FindScene(const std::string& name)
    {
        for (auto& s : scenes_)
        {
            if (s->m_name == name)
            {
                return s.get();
            }
        }
        return nullptr;
    }

    void Engine::SetActiveScene(const std::string& name)
    {
        if (auto* s = FindScene(name))
        {
            m_activeScene = s;
        }
    }

    // --------------------------------------------------------------
    // Main Tick / Frame Loop
    // --------------------------------------------------------------

    void Engine::Tick(float dt)
    {
        if (m_activeScene)
        {
            m_activeScene->update(dt);
        }

        // 1) Clear swapchain (animated color demo)
        static float t_ = 0.f;
        t_ += dt;
        const float r = 0.10f + 0.05f * std::sin(t_ * 1.7f);
        const float g = 0.12f + 0.05f * std::sin(t_ * 1.3f + 1.0f);
        const float b = 0.16f + 0.05f * std::sin(t_ * 0.9f + 2.0f);
        BeginFrame(gfx_, r, g, b, 1.0f);

        if (m_activeScene)
        {
            RenderScene(*m_activeScene);
        }

        // 3) Global ImGui overlay
        if (ui_.enabled) {
            BeginUiFrame(dt);
            RenderUi(gfx_.ctx);
        }

        // 4) Present
        EndFrame(gfx_);
    }

    // --------------------------------------------------------------
    // ImGui Management
    // --------------------------------------------------------------

    bool Engine::EnableImGui(SDL_Window* sdlWindow, bool docking, bool multiViewport)
    {
        if (ui_.enabled)
            return true;
        if (!gfx_.device || !gfx_.swapchain)
            return false;

        ui_.sdlWindow = sdlWindow;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        if (docking)
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        if (multiViewport)
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        ImGui::StyleColorsDark();

        // SDL input backend (rendering happens via Diligent)
        ImGui_ImplSDL2_InitForOther(ui_.sdlWindow);

        // Diligent ImGui renderer
        const auto& scDesc = gfx_.swapchain->GetDesc();
        ImGuiDiligentCreateInfo ci{ gfx_.device, scDesc };
        ui_.imguiDiligent = new ImGuiImplDiligent(ci);

        ui_.enabled = true;
        return true;
    }

    void Engine::DisableImGui()
    {
        if (!ui_.enabled)
            return;

        if (ui_.imguiDiligent) {
            delete ui_.imguiDiligent;
            ui_.imguiDiligent = nullptr;
        }

        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        ui_.enabled = false;
    }

    void Engine::ProcessSDLEvent(const SDL_Event& e)
    {
        if (ui_.enabled)
            ImGui_ImplSDL2_ProcessEvent(&e);
    }

    void Engine::BeginUiFrame(float /*dt*/)
    {
        ImGui_ImplSDL2_NewFrame();

        const auto scDesc = gfx_.swapchain->GetDesc();
        ui_.imguiDiligent->NewFrame(
            static_cast<Uint32>(scDesc.Width),
            static_cast<Uint32>(scDesc.Height),
            scDesc.PreTransform
        );
    }

    void Engine::RenderUi(IDeviceContext* ctx)
    {
        ui_.imguiDiligent->Render(ctx);
    }

    // --------------------------------------------------------------
    // Scene Rendering
    // --------------------------------------------------------------

    void RenderScene(Scene& scene)
    {
        // Find all cameras and sort by priority
        std::vector<std::pair<int, flecs::entity>> cameras;
        scene.m_world.each<CameraComponent>(
            [&](flecs::entity e, CameraComponent& cam)
            {
                cameras.emplace_back(cam.priority, e);
            });

        std::sort(cameras.begin(), cameras.end(),
            [](auto& a, auto& b)
            { 
                return a.first < b.first;
            });

        // Render all cameras in order
        for (auto& [_, entity] : cameras) {
            auto& cam = *entity.get_mut<CameraComponent>();
            if (!cam.targetRTV || !cam.targetDSV || !cam.deviceCtx)
            {
                continue;
            }

            ITextureView* rtvs[] = { cam.targetRTV.RawPtr<>() };
            cam.deviceCtx->SetRenderTargets(1, rtvs, cam.targetDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            const float clear[4] = { 0.05f, 0.05f, 0.08f, 1.0f };
            cam.deviceCtx->ClearRenderTarget(rtvs[0], clear, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            cam.deviceCtx->ClearDepthStencil(cam.targetDSV, CLEAR_DEPTH_FLAG, 1.0f, 0,
                RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            // Draw entities that provide a render callback (fast path, no sorting here)
            scene.m_world.each<RenderProxy>(
                [&](flecs::entity /*e*/, RenderProxy& rp)
                {
                    if (rp.draw)
                    {
                        rp.draw(cam.deviceCtx);
                    }
                });

            // UI overlay
            if (cam.canvas.is_alive())
            {
                auto canvasComp = cam.canvas.get<CanvasComponent>();
                if (canvasComp && canvasComp->uiLayerFn)
                {
                    canvasComp->uiLayerFn();
                }
            }
        }
    }

    // --------------------------------------------------------------
    // Frame Utilities
    // --------------------------------------------------------------

    bool CreateDeviceAndSwapchain(const NativeWindow& win, GfxContext& out)
    {
#if defined(_WIN32)
        EngineD3D12CreateInfo engCI{};
        RefCntAutoPtr<IEngineFactoryD3D12> factory{ GetEngineFactoryD3D12() };
        if (!factory)
        {
            return false;
        }

        SwapChainDesc scDesc{};
        scDesc.Width = static_cast<Uint32>(win.width);
        scDesc.Height = static_cast<Uint32>(win.height);
        scDesc.ColorBufferFormat = TEX_FORMAT_RGBA8_UNORM_SRGB;
        scDesc.DepthBufferFormat = TEX_FORMAT_D32_FLOAT;

        Win32NativeWindow wnd{};
        wnd.hWnd = static_cast<HWND>(win.hWnd);

        RefCntAutoPtr<IRenderDevice> device;
        RefCntAutoPtr<IDeviceContext> ctx;
        RefCntAutoPtr<ISwapChain> sc;

        factory->CreateDeviceAndContextsD3D12(engCI, &device, &ctx);
        factory->CreateSwapChainD3D12(device, ctx, scDesc, FullScreenModeDesc{}, wnd, &sc);

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
        if (!gfx.swapchain || !gfx.ctx)
            return;

        auto* pRTV = gfx.swapchain->GetCurrentBackBufferRTV();
        auto* pDSV = gfx.swapchain->GetDepthBufferDSV();

        const float clear[4] = { r, g, b, a };
        gfx.ctx->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        gfx.ctx->ClearRenderTarget(pRTV, clear, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        if (pDSV)
        {
            gfx.ctx->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.0f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        }
    }

    void EndFrame(GfxContext& gfx)
    {
        if (gfx.swapchain)
        {
            gfx.swapchain->Present();
        }
    }
} // namespace NexusEngine
