#pragma once

#include <memory>
#include <NexusEngine.h>

#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
#if defined(SAMPLEGAME_EXPORTS)
#define SAMPLEGAME_API __declspec(dllexport)
#else
#define SAMPLEGAME_API __declspec(dllimport)
#endif
#else
#define SAMPLEGAME_API
#endif

namespace SampleGame
{
    extern "C"
    {
        /// <summary>
        /// Creates the sample game implementation used by dynamic loading.
        /// </summary>
        /// <returns>The created game implementation.</returns>
        SAMPLEGAME_API NexusEngine::IGameApp* LoadGame();

        /// <summary>
        /// Destroys a game implementation created by dynamic loading.
        /// </summary>
        /// <param name="game">Game instance to destroy.</param>
        SAMPLEGAME_API void UnloadGame(NexusEngine::IGameApp* game);
    }

    /// <summary>
    /// Creates the sample game implementation used by the runtime.
    /// </summary>
    /// <returns>The created game implementation.</returns>
    SAMPLEGAME_API std::unique_ptr<NexusEngine::IGameApp> CreateGame();
} // namespace SampleGame
