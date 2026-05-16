#pragma once

#include <flecs.h>

#include <string>

namespace NexusEngine
{
    class Engine;
    class RenderResourceFactory;

    struct Scene
    {
        /// <summary>
        /// Creates a scene wrapper whose entities live under a root entity in the engine world.
        /// </summary>
        /// <param name="engine">Owning engine instance.</param>
        /// <param name="name">Initial scene name.</param>
        explicit Scene(Engine& engine, const std::string& name = "Unnamed");

        /// <summary>
        /// Releases scene-owned entities from the engine world.
        /// </summary>
        ~Scene();

        /// <summary>
        /// Runs one scene update.
        /// </summary>
        /// <param name="dt">Frame delta time in seconds.</param>
        void Update(float dt);

        /// <summary>
        /// Creates a new entity under the scene root.
        /// </summary>
        /// <param name="name">Optional entity name.</param>
        /// <returns>The created entity handle.</returns>
        flecs::entity CreateEntity(const char* name = nullptr);

        /// <summary>
        /// Destroys an entity if it belongs to this scene and is still alive.
        /// </summary>
        /// <param name="entity">Entity to destroy.</param>
        void DestroyEntity(flecs::entity entity);

        /// <summary>
        /// Returns whether an entity belongs to this scene hierarchy.
        /// </summary>
        /// <param name="entity">Entity to inspect.</param>
        /// <returns>True if the entity is in this scene; otherwise false.</returns>
        bool ContainsEntity(const flecs::entity& entity) const;

        /// <summary>
        /// Finds a named entity within this scene hierarchy.
        /// </summary>
        /// <param name="name">Entity name to locate.</param>
        /// <returns>The matching entity, or an invalid entity when none exists.</returns>
        flecs::entity FindEntityByName(const char* name) const;

        /// <summary>
        /// Returns the engine-owned world backing this scene.
        /// </summary>
        /// <returns>The shared engine world.</returns>
        flecs::world& GetWorld() const;

        /// <summary>
        /// Returns the scene root entity.
        /// </summary>
        /// <returns>The scene root entity.</returns>
        flecs::entity GetRootEntity() const { return m_rootEntity; }

        /// <summary>
        /// Returns the scene resource factory for meshes, materials, and shaders.
        /// </summary>
        /// <returns>The scene render resource factory.</returns>
        RenderResourceFactory* GetResourceFactory() const;

        std::string m_name;

    private:
        Engine* m_engine = nullptr;
        flecs::entity m_rootEntity;
    };
} // namespace NexusEngine
