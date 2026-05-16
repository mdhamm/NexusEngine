#pragma once

#include <memory>
#include <NexusEngine.h>

namespace SampleGame
{
    /// <summary>
    /// Registers the sample game component descriptors with the editor reflection registry.
    /// </summary>
    void RegisterEditorComponentDescriptors();

    /// <summary>
    /// Creates the sample game implementation used by the runtime.
    /// </summary>
    /// <returns>The created game implementation.</returns>
    std::unique_ptr<NexusEngine::IGameApp> CreateGame();

    /// <summary>
    /// Registers gameplay systems into the engine world and returns the created system entities.
    /// </summary>
    /// <param name="engine">Engine owning the gameplay world.</param>
    /// <returns>The created gameplay system entities.</returns>
    std::vector<flecs::entity> RegisterSystems(NexusEngine::Engine& engine);
} // namespace SampleGame
