#pragma once
#include <flecs.h>
#include <string>
#include <vector>
#include <memory>
#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Graphics/GraphicsEngine/interface/Buffer.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/TextureView.h>

#include "rendering/GraphicsRenderer.h"

namespace NexusEngine
{
    struct CameraComponent;
    class RenderResourceFactory;

    // Marker for the input phase in the scene pipeline.
    struct InputPhase {};

    // Marker for the gameplay phase in the scene pipeline.
    struct GameplayPhase {};

    // Marker for the physics phase in the scene pipeline.
    struct PhysicsPhase {};

    // Marker for the transform phase in the scene pipeline.
    struct TransformPhase {};

    // Marker for the animation phase in the scene pipeline.
    struct AnimationPhase {};

    // Marker for the visibility phase in the scene pipeline.
    struct VisibilityPhase {};

    // Marker for the render preparation phase in the scene pipeline.
    struct RenderPrepPhase {};

    // Marker for the main render phase in the scene pipeline.
    struct RenderPhase {};

    // Marker for the post-render phase in the scene pipeline.
    struct RenderPostPhase {};

    // Lightweight scene wrapper around a Flecs world and render resources.
    struct Scene
    {
        /// <summary>
        /// Creates a scene with its own ECS world and render resource factory.
        /// </summary>
        /// <param name="graphicsRenderer">Renderer used to create scene resources.</param>
        /// <param name="name">Initial scene name.</param>
        Scene(GraphicsRenderer& graphicsRenderer, const std::string& name = "Unnamed");

        /// <summary>
        /// Releases scene-owned resources.
        /// </summary>
        ~Scene();

        /// <summary>
        /// Runs one scene update.
        /// </summary>
        /// <param name="dt">Frame delta time in seconds.</param>
        void Update(float dt);

        /// <summary>
        /// Creates a new entity in the scene world.
        /// </summary>
        /// <param name="name">Optional entity name.</param>
        /// <returns>The created entity handle.</returns>
        inline flecs::entity CreateEntity(const char* name = nullptr)
        {
            return m_world.entity(name);
        }

        /// <summary>
        /// Destroys an entity if it is still alive.
        /// </summary>
        /// <param name="e">Entity to destroy.</param>
        inline void DestroyEntity(flecs::entity e)
        {
            if (e.is_alive()) e.destruct();
        }

        /// <summary>
        /// Returns the scene resource factory for meshes, materials, and shaders.
        /// </summary>
        /// <returns>The scene render resource factory.</returns>
        RenderResourceFactory* GetResourceFactory() const { return m_resourceFactory.get(); }

        // Scene display name.
        std::string m_name;

        // ECS world that stores entities, components, and systems.
        flecs::world m_world;
    private:
        void RegisterSceneComponents();
        void RegisterPhases();
        void RegisterSystems();
        bool EnsureInstanceTransformBufferCapacity(Diligent::Uint32 instanceCount);

        GraphicsRenderer m_graphicsRenderer;
        std::unique_ptr<RenderResourceFactory> m_resourceFactory;
        Diligent::RefCntAutoPtr<Diligent::IBuffer> m_instanceTransformBuffer;
        Diligent::Uint32 m_instanceTransformCapacity = 0;
        float m_clearAnimationTime = 0.0f;
    };

} // namespace NexusEngine
