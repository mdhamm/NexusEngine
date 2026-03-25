#pragma once
#include <flecs.h>
#include <string>
#include <vector>
#include <DiligentCore/Graphics/GraphicsEngine/interface/TextureView.h>

namespace NexusEngine
{
    struct CameraComponent;
    struct CanvasComponent;

    // A lightweight scene wrapper around a Flecs world.
    struct Scene
    {
        std::string m_name;
        flecs::world m_world;

        Scene(const std::string& name = "Unnamed")
            : m_name(name) {}

        void update(float dt);

        inline flecs::entity createEntity(const char* name = nullptr)
        {
            return m_world.entity(name);
        }

        inline void destroyEntity(flecs::entity e)
        {
            if (e.is_alive()) e.destruct();
        }
    };

    // Scene-global registration
    void RegisterSceneComponents(flecs::world& w);

    // Render entry point
    void RenderScene(Scene& scene);
} // namespace NexusEngine
