#include "EditorSceneApp.h"

#include <Scene.h>

namespace NexusEditor
{
    std::vector<flecs::entity> EditorSceneApp::RegisterSystems(NexusEngine::Engine& engine)
    {
        REF(engine);
        return {};
    }

    void EditorSceneApp::OnStartup(NexusEngine::Engine& engine)
    {
        REF(engine);
    }
} // namespace NexusEditor
