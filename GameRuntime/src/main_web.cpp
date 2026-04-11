#include <SDL.h>
#include <emscripten/emscripten.h>

#include <memory>

#include <NexusEngine.h>
#include <Game.h>

static NexusEngine::Engine g_engine;
static SDL_Window*         g_window = nullptr;

static double g_prev_ms = 0.0;

static void main_loop()
{
    const double now_ms = emscripten_get_now();
    double dt = (g_prev_ms > 0.0) ? (now_ms - g_prev_ms) / 1000.0 : (1.0 / 60.0);
    g_prev_ms = now_ms;

    if (dt > 0.1) dt = 0.1; // clamp
    g_engine.RunFrame(static_cast<float>(dt));
}

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
        return 1;

    g_window = SDL_CreateWindow(
        "GameRuntime (Web)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!g_window) return 1;

    std::unique_ptr<NexusEngine::IGameApp> game = SampleGame::CreateGame();
    if (!game)
        return 1;

    NexusEngine::NativeWindow nw{};
    nw.m_width = 1280;
    nw.m_height = 720;
    nw.m_canvasId = "#canvas";

    if (!g_engine.Initialize(nw, std::move(game)))
        return 1;

    g_prev_ms = emscripten_get_now();
    emscripten_set_main_loop(main_loop, 0, 1);
    return 0;
}
