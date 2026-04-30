#pragma once

#include <NexusEngine.h>

namespace NexusEditor
{
    class EditorSceneApp final : public NexusEngine::IGameApp
    {
    public:
        /// <summary>
        /// Creates the default editor-owned scene when the editor engine starts.
        /// </summary>
        /// <param name="engine">Engine instance used to create the editor scene.</param>
        void OnStartup(NexusEngine::Engine& engine) override;
    };
} // namespace NexusEditor
