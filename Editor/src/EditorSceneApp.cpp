#include "EditorSceneApp.h"

#include <Scene.h>

namespace NexusEditor
{
    void EditorSceneApp::OnStartup(NexusEngine::Engine& engine)
    {
        NexusEngine::Scene& scene = engine.CreateScene("EditorScene");
        engine.SetActiveScene("EditorScene");

        (void)scene;
    }
} // namespace NexusEditor
