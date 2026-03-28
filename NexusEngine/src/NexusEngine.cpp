#include "NexusEngine.h"
#include "Scene.h"

//#include <backends/imgui_impl_sdl2.h>

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
        bool graphicsInitialized = m_graphicsRenderer.CreateDeviceAndSwapchain(win);
        m_initialized = graphicsInitialized;
        return graphicsInitialized;
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

        m_scenes.clear();
    }

    // --------------------------------------------------------------
    // Scene Management
    // --------------------------------------------------------------

    Scene& Engine::CreateScene(const std::string& name)
    {
        auto s = std::make_unique<Scene>(m_graphicsRenderer, name);
        Scene& ref = *s;
        m_scenes.push_back(std::move(s));
        return ref;
    }

    void Engine::RemoveScene(const std::string& name)
    {
        m_scenes.erase(std::remove_if(m_scenes.begin(), m_scenes.end(),
            [&](auto& sc)
            {
                return sc->m_name == name;
            }),
            m_scenes.end());
    }

    Scene* Engine::FindScene(const std::string& name)
    {
        for (auto& s : m_scenes)
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
            m_activeScene->Update(dt);
        }

        // Clear swapchain (animated color demo)
        static float t_ = 0.f;
        t_ += dt;
        const float r = 0.10f + 0.05f * std::sin(t_ * 1.7f);
        const float g = 0.12f + 0.05f * std::sin(t_ * 1.3f + 1.0f);
        const float b = 0.16f + 0.05f * std::sin(t_ * 0.9f + 2.0f);
        m_graphicsRenderer.BeginFrame(r, g, b, 1.0f);
        
        if (m_activeScene)
        {
            m_activeScene->Render();
        }

        m_graphicsRenderer.EndFrame();
    }

    // --------------------------------------------------------------
    // ImGui Management
    // --------------------------------------------------------------

    //bool Engine::EnableImGui(SDL_Window* sdlWindow, bool docking, bool multiViewport)
    //{
    //    if (m_ui.enabled)
    //        return true;
    //    if (!m_gfx.m_device || !m_gfx.m_swapchain)
    //    {
    //        return false;
    //    }

    //    m_ui.sdlWindow = sdlWindow;

    //    IMGUI_CHECKVERSION();
    //    ImGui::CreateContext();
    //    ImGuiIO& io = ImGui::GetIO();
    //    if (docking)
    //        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    //    if (multiViewport)
    //        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    //    ImGui::StyleColorsDark();

    //    // SDL input backend (rendering happens via Diligent)
    //    ImGui_ImplSDL2_InitForOther(m_ui.sdlWindow);

    //    // Diligent ImGui renderer
    //    const auto& scDesc = m_gfx.m_swapchain->GetDesc();
    //    ImGuiDiligentCreateInfo ci{ m_gfx.device, scDesc };
    //    m_ui.imguiDiligent = new ImGuiImplDiligent(ci);

    //    m_ui.enabled = true;
    //    return true;
    //}

    //void Engine::DisableImGui()
    //{
    //    if (!m_ui.enabled)
    //    {
    //        return;
    //    }

    //    if (m_ui.imguiDiligent)
    //    {
    //        delete m_ui.imguiDiligent;
    //        m_ui.imguiDiligent = nullptr;
    //    }

    //    ImGui_ImplSDL2_Shutdown();
    //    ImGui::DestroyContext();

    //    m_ui.enabled = false;
    //}

    void Engine::ProcessSDLEvent(const SDL_Event& e)
    {
        //if (m_ui.enabled)
        //    ImGui_ImplSDL2_ProcessEvent(&e);
    }

    //void Engine::BeginUiFrame(float /*dt*/)
    //{
    //    ImGui_ImplSDL2_NewFrame();

    //    const Diligent::SwapChainDesc& scDesc = m_gfx.m_swapchain->GetDesc();
    //    m_ui.imguiDiligent->NewFrame(
    //        static_cast<Uint32>(scDesc.Width),
    //        static_cast<Uint32>(scDesc.Height),
    //        scDesc.PreTransform
    //    );
    //}

    //void Engine::RenderUi(IDeviceContext* ctx)
    //{
    //    m_ui.imguiDiligent->Render(ctx);
    //}
    //}
} // namespace NexusEngine
