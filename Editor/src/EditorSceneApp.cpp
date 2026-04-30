#include "EditorSceneApp.h"

#include <Scene.h>
#include <components/CameraComponent.h>
#include <components/TransformComponent.h>

namespace NexusEditor
{
    void EditorSceneApp::OnStartup(NexusEngine::Engine& engine)
    {
        NexusEngine::Scene& scene = engine.CreateScene("EditorScene");
        engine.SetActiveScene("EditorScene");

        scene.m_world.entity("EditorCamera")
            .set(NexusEngine::CameraComponent{})
            .set(NexusEngine::TransformComponent::FromLocal(
                Diligent::float3(0.0f, 0.0f, 10.0f),
                NexusEngine::Quaternion::FromEuler(0.0f, 0.0f, 0.0f),
                Diligent::float3(1.0f, 1.0f, 1.0f)));
    }
} // namespace NexusEditor
