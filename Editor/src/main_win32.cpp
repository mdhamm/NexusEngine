#include <SDL.h>
#include <SDL_syswm.h>
#include <cstdio>
#include <NexusEngine.h>

static void ReportError(const char* title, const char* detail)
{
    if (SDL_WasInit(0) != 0)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, detail, nullptr);
    }
    std::fprintf(stderr, "%s: %s\n", title, detail ? detail : "(no detail)");
}

int main(int, char**)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0)
    {
        ReportError("SDL_Init failed", SDL_GetError());
        return 1;
    }

    const int winW = 1600, winH = 900;
    SDL_Window* win = SDL_CreateWindow(
        "Nexus Editor",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        winW, winH,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!win)
    {
        ReportError("SDL_CreateWindow failed", SDL_GetError());
        return 1;
    }

    SDL_SysWMinfo wmi{};
    SDL_VERSION(&wmi.version);
    if (!SDL_GetWindowWMInfo(win, &wmi))
    {
        ReportError("SDL_GetWindowWMInfo failed", SDL_GetError());
        SDL_DestroyWindow(win); SDL_Quit(); return 1;
    }

    NexusEngine::AppContext ctx{};
    ctx.width = winW; ctx.height = winH; ctx.nativeWindow = wmi.info.win.window;

    NexusEngine::Engine engine;
    if (!engine.Initialize({ winW, winH, ctx.nativeWindow }))
    {
        ReportError("Engine init failed", "Diligent device/swapchain");
        SDL_DestroyWindow(win); SDL_Quit(); return 1;
    }

    // Enable ImGui inside the Engine
    engine.EnableImGui(win, /*docking*/true, /*multiViewport*/true);

    // Create an offscreen viewport we’ll show inside a panel
    NexusEngine::ViewportId view = engine.CreateOffscreenViewport({ 1280, 720 });

    // Register a single UI layer that draws a dockspace and the scene Viewport
    engine.AddUiLayer(
        [&]() {
            // Dockspace
            ImGuiWindowFlags flags =
                ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar;

            const ImGuiViewport* vp = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(vp->WorkPos);
            ImGui::SetNextWindowSize(vp->WorkSize);
            ImGui::SetNextWindowViewport(vp->ID);

            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
            ImGui::Begin("Dockspace", nullptr, flags);
            ImGui::PopStyleVar(2);
            ImGui::DockSpace(ImGui::GetID("EditorDockspace"), ImVec2(0, 0),
                ImGuiDockNodeFlags_PassthruCentralNode);
            ImGui::End();

            // Simple menu
            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("View"))
                {
                    static bool showDemo = false;
                    ImGui::MenuItem("ImGui Demo", nullptr, &showDemo);
                    if (showDemo) ImGui::ShowDemoWindow(&showDemo);
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

            // Scene Viewport panel
            ImGui::Begin("Viewport");
            ImVec2 avail = ImGui::GetContentRegionAvail();
            if (avail.x < 1)
            {
                avail.x = 1;
            }
            
            if (avail.y < 1)
            {
                avail.y = 1;
            }

            // Resize offscreen RT if panel size changed
            static ImVec2 last = { 0,0 };
            if (last.x != avail.x || last.y != avail.y)
            {
                engine.ResizeOffscreenViewport(view, (uint32_t)avail.x, (uint32_t)avail.y);
                last = avail;
            }

            // Basic camera
            const float aspect = avail.x / (avail.y > 0 ? avail.y : 1.0f);
            Diligent::float4x4 proj = Diligent::float4x4::Projection(
                60.0f * Diligent::PI_F / 180.0f, aspect, 0.1f, 1000.0f, false);
            Diligent::float4x4 viewM = Diligent::float4x4::Translation(0.f, 0.f, -5.f);

            // Ask engine to render into offscreen RT
            engine.RenderSceneToViewport(view, viewM, proj);

            // Display it
            auto* srv = engine.GetViewportSRV(view);
            ImGui::Image((ImTextureID)srv, avail);

            ImGui::End();
        });

    using Clock = std::chrono::high_resolution_clock;
    auto prev = Clock::now();
    bool running = true;

    while (running)
    {
        SDL_Event e{};
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                running = false;
            }
            engine.ProcessSDLEvent(e); // forward to ImGui inside Engine
        }

        auto now = Clock::now();
        float dt = std::chrono::duration<float>(now - prev).count();
        prev = now; if (dt > 0.1f) dt = 0.1f;

        engine.Tick(dt);
        SDL_Delay(1);
    }

    engine.Shutdown();
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
