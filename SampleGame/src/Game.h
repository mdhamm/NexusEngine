#pragma once
#include <memory>
#include <NexusEngine.h>

namespace SampleGame
{
    /// <summary>
    /// Creates the sample game implementation used by the runtime.
    /// </summary>
    /// <returns>The created game implementation.</returns>
    std::unique_ptr<NexusEngine::IGameApp> CreateGame();
} // namespace SampleGame
