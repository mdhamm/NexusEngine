#pragma once
#include <memory>
#include <NexusEngine.h>

namespace SampleGame
{
    std::unique_ptr<NexusEngine::IGameApp> CreateGame();
} // namespace SampleGame
