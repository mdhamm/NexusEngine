#include <SDL.h>
#include <SDL_syswm.h>
#include <cstdio>
#include <functional>

#include <NexusEngine.h>
#include <Game.h>

static void ReportError(const char* title, const char* detail)
{
    // Try an SDL message box if SDL is initialized, otherwise just print.
    if (SDL_WasInit(0) != 0)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, detail, nullptr);
    }
    std::fprintf(stderr, "%s: %s\n", title, detail ? detail : "(no detail)");
}

int main(int argc, char** argv)
{
    REF(argc);
    REF(argv);

    // 1) Init SDL (bootstrap concern)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0)
    {
        ReportError("SDL_Init failed", SDL_GetError());
        return 1;
    }

    // 2) Create window (bootstrap concern)
    const int winW = 1280;
    const int winH = 720;
    SDL_Window* win = SDL_CreateWindow(
        "GameRuntime (D3D12)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        winW, winH,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!win)
    {
        ReportError("SDL_CreateWindow failed", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // 3) Extract native HWND for the engine (bootstrap concern)
    SDL_SysWMinfo wmi{};
    SDL_VERSION(&wmi.version);
    if (!SDL_GetWindowWMInfo(win, &wmi))
    {
        ReportError("SDL_GetWindowWMInfo failed", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }
    HWND hWnd = wmi.info.win.window;

    std::unique_ptr<NexusEngine::IGameApp> game = SampleGame::CreateGame();
    if (!game)
    {
        ReportError("Factory creation failed", "SampleGame::CreateFactory returned null");
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    // 6) Provide a message pump to the Engine::Run loop
    auto pump = [win]() -> bool
        {
            bool running = true;
            SDL_Event e{};
            while (SDL_PollEvent(&e)) {
                switch (e.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_WINDOWEVENT:
                    if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                        e.window.event == SDL_WINDOWEVENT_RESIZED)
                    {
                        // int newW = e.window.data1;
                        // int newH = e.window.data2;
                        // Optional: notify your engine/game via a future resize API.
                    }
                    break;
                default:
                    break;
                }
            }
            // Optional: tiny sleep to avoid 100% CPU when minimized (keep simple here)
            SDL_Delay(1);
            return running;
        };

    NexusEngine::NativeWindow nativeWindow;
    nativeWindow.width = winW;
    nativeWindow.height = winH;
    nativeWindow.hWnd = hWnd;

    NexusEngine::Engine engine;
    engine.Initialize(nativeWindow, std::move(game));
    engine.Start();
    engine.Shutdown();

    // 8) Cleanup bootstrap
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
