#pragma once

#include <NexusEngine.h>

namespace NexusEditor
{
    class EditorSceneApp final : public NexusEngine::IGameApp
    {
    public:
        /// <summary>
        /// Registers editor-owned systems into the engine world and returns the created system entities.
        /// </summary>
        /// <param name="engine">Engine owning the editor world.</param>
        /// <returns>The created editor system entities.</returns>
        std::vector<flecs::entity> RegisterSystems(NexusEngine::Engine& engine) override;

        /// <summary>
        /// Creates the default editor-owned scene when the editor engine starts.
        /// </summary>
        /// <param name="engine">Engine instance used to create the editor scene.</param>
        void OnStartup(NexusEngine::Engine& engine) override;
    };

} // namespace NexusEditor
