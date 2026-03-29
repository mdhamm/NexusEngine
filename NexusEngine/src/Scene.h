#pragma once
#include <flecs.h>
#include <string>
#include <vector>
#include <memory>
#include <DiligentCore/Graphics/GraphicsEngine/interface/TextureView.h>

#include "common/GraphicsRenderer.h"

namespace NexusEngine
{
    struct CameraComponent;
    class RenderResourceFactory;

    // Engine pipeline phases
    struct InputPhase {};
    struct GameplayPhase {};
    struct PhysicsPhase {};
    struct TransformPhase {};
    struct AnimationPhase {};
    struct VisibilityPhase {};
    struct RenderPrepPhase {};
    struct RenderPhase {};
    struct RenderPostPhase {};

    // A lightweight scene wrapper around a Flecs world.
    struct Scene
    {
        Scene(GraphicsRenderer& graphicsRenderer, const std::string& name = "Unnamed");
        ~Scene();

        void Update(float dt);

        inline flecs::entity CreateEntity(const char* name = nullptr)
        {
            return m_world.entity(name);
        }

        inline void DestroyEntity(flecs::entity e)
        {
            if (e.is_alive()) e.destruct();
        }

        // Get resource factory for creating meshes, materials, etc.
        RenderResourceFactory* GetResourceFactory() const { return m_resourceFactory.get(); }

        std::string m_name;
        flecs::world m_world;
    private:
        void RegisterSceneComponents();
        void RegisterPhases();
        void RegisterSystems();

        GraphicsRenderer m_graphicsRenderer;
        std::unique_ptr<RenderResourceFactory> m_resourceFactory;
        float m_clearAnimationTime = 0.0f;
    };

} // namespace NexusEngine
