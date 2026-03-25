#include <SDL.h>
#include <emscripten/emscripten.h>
#include <stdexcept>
#include <chrono>

#include <NexusEngine.h>
#include <Game.h>

static NexusEngine::Engine g_engine;
static SampleGame::Game    g_game;

static double g_prev_ms = 0.0;

static void main_loop()
{
    const double now_ms = emscripten_get_now();
    double dt = (g_prev_ms > 0.0) ? (now_ms - g_prev_ms) / 1000.0 : (1.0 / 60.0);
    g_prev_ms = now_ms;

    if (dt > 0.1) dt = 0.1; // clamp
    g_game.Update(g_engine, static_cast<float>(dt));
}

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
        return 1;

    SDL_Window* win = SDL_CreateWindow(
        "GameRuntime (WebGPU)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_RESIZABLE
    );
    if (!win) return 1;

    NexusEngine::NativeWindow nw{};
    nw.width = 1280;
    nw.height = 720;

    if (!g_engine.Initialize(nw))
        return 1;

    g_game.Initialize(g_engine);

    g_prev_ms = emscripten_get_now();
    emscripten_set_main_loop(main_loop, 0, 1);
    return 0;
}
