#include "Scene.h"

#include "NexusEngine.h"

namespace NexusEngine
{
    Scene::Scene(Engine& engine, const std::string& name)
        : m_name(name)
        , m_engine(&engine)
    {
        flecs::world& world = GetWorld();
        m_rootEntity = world.entity(name.c_str());
    }

    Scene::~Scene()
    {
        if (m_rootEntity.is_valid() && m_rootEntity.is_alive())
        {
            m_rootEntity.destruct();
        }
    }

    void Scene::Update(float dt)
    {
        REF(dt);
    }

    flecs::entity Scene::CreateEntity(const char* name)
    {
        flecs::entity entity = name && name[0] != '\0'
            ? GetWorld().entity(name)
            : GetWorld().entity();
        entity.child_of(m_rootEntity);
        return entity;
    }

    void Scene::DestroyEntity(flecs::entity entity)
    {
        if (ContainsEntity(entity) && entity.is_alive())
        {
            entity.destruct();
        }
    }

    bool Scene::ContainsEntity(const flecs::entity& entity) const
    {
        if (!entity.is_valid() || !m_rootEntity.is_valid())
        {
            return false;
        }

        flecs::entity current = entity;
        while (current.is_valid())
        {
            if (current == m_rootEntity)
            {
                return true;
            }

            current = current.parent();
        }

        return false;
    }

    flecs::entity Scene::FindEntityByName(const char* name) const
    {
        if (!name || name[0] == '\0')
        {
            return {};
        }

        flecs::entity found;
        m_rootEntity.children(
            [&](flecs::entity child)
            {
                if (found.is_valid())
                {
                    return;
                }

                if (child.name() && std::string_view(child.name()) == name)
                {
                    found = child;
                }
            });
        return found;
    }

    flecs::world& Scene::GetWorld() const
    {
        return m_engine->GetWorld();
    }

    RenderResourceFactory* Scene::GetResourceFactory() const
    {
        return m_engine ? m_engine->GetResourceFactory() : nullptr;
    }
} // namespace NexusEngine

