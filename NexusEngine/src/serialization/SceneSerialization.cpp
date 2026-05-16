#include "SceneSerialization.h"

#include "EntitySerialization.h"
#include "ISerializer.h"
#include "components/EditorOnlyComponent.h"
#include "components/RenderMeshComponent.h"
#include "components/TransformComponent.h"
#include "rendering/Material.h"
#include "rendering/Mesh.h"
#include "rendering/RenderResourceFactory.h"

#include <unordered_map>

namespace NexusEngine
{
    namespace
    {
        void AssignDefaultRenderResources(Scene& scene)
        {
            RenderResourceFactory* factory = scene.GetResourceFactory();
            if (!factory)
            {
                return;
            }

            Mesh* cubeMesh = nullptr;
            Material* unlitMaterial = nullptr;
            flecs::world& world = scene.GetWorld();

            world.each<RenderMeshComponent>(
                [&](flecs::entity entity, RenderMeshComponent& renderMesh)
                {
                    if (!scene.ContainsEntity(entity))
                    {
                        return;
                    }

                    if (renderMesh.mesh && (renderMesh.material || !renderMesh.m_materialAssetReference.IsEmpty()))
                    {
                        return;
                    }

                    if (!cubeMesh)
                    {
                        cubeMesh = factory->CreateCubeMesh();
                    }

                    if (!unlitMaterial)
                    {
                        unlitMaterial = factory->CreateUnlitMaterial();
                    }

                    if (!renderMesh.mesh)
                    {
                        renderMesh.mesh = cubeMesh;
                    }

                    if (!renderMesh.material && renderMesh.m_materialAssetReference.IsEmpty())
                    {
                        renderMesh.material = unlitMaterial;
                    }
                });
        }
    }

    void Serialize(const Scene& scene, ISerializeWriter& writer)
    {
        writer.Write("name", scene.m_name);
        writer.BeginArray("entities");
        scene.GetRootEntity().children(
            [&](flecs::entity entity)
            {
                if (!entity.is_valid() || !entity.is_alive() || !entity.has<TransformComponent>() || entity.has<EditorOnlyComponent>())
                {
                    return;
                }

                writer.BeginArrayElement();
                SerializeEntity(entity, scene.GetRootEntity(), writer);
                writer.EndArrayElement();
            });
        writer.EndArray();
        writer.EndObject();
    }

    bool Deserialize(Scene& scene, ISerializeReader& reader)
    {
        reader.Read("name", scene.m_name);

        if (!HasObjectKey(reader, "entities"))
        {
            return true;
        }

        struct PendingEntity
        {
            flecs::entity m_entity;
            std::uint64_t m_parentId = 0;
        };

        std::unordered_map<std::uint64_t, PendingEntity> pendingEntities;
        const std::size_t entityCount = reader.GetArraySize("entities");
        pendingEntities.reserve(entityCount);

        reader.BeginArray("entities");
        for (std::size_t entityIndex = 0; entityIndex < entityCount; ++entityIndex)
        {
            reader.BeginArrayElement(entityIndex);
            SerializedEntityHeader header;
            if (DeserializeEntityHeader(header, reader))
            {
                PendingEntity pendingEntity;
                pendingEntity.m_entity = header.m_name.empty()
                    ? scene.CreateEntity()
                    : scene.CreateEntity(header.m_name.c_str());
                pendingEntity.m_parentId = header.m_parentId;
                pendingEntities.emplace(header.m_id, std::move(pendingEntity));
            }
            reader.EndArrayElement();
        }
        reader.EndArray();

        reader.BeginArray("entities");
        for (std::size_t entityIndex = 0; entityIndex < entityCount; ++entityIndex)
        {
            reader.BeginArrayElement(entityIndex);
            std::uint64_t originalId = 0;
            reader.Read("id", originalId);
            const auto pendingIt = pendingEntities.find(originalId);
            if (pendingIt != pendingEntities.end())
            {
                (void)DeserializeEntity(pendingIt->second.m_entity, reader);
            }
            reader.EndArrayElement();
        }
        reader.EndArray();

        for (auto& [originalId, pendingEntity] : pendingEntities)
        {
            (void)originalId;
            if (pendingEntity.m_parentId == 0)
            {
                pendingEntity.m_entity.child_of(scene.GetRootEntity());
                continue;
            }

            const auto parentIt = pendingEntities.find(pendingEntity.m_parentId);
            if (parentIt != pendingEntities.end())
            {
                pendingEntity.m_entity.child_of(parentIt->second.m_entity);
            }
            else
            {
                pendingEntity.m_entity.child_of(scene.GetRootEntity());
            }
        }

        AssignDefaultRenderResources(scene);
        return true;
    }
} // namespace NexusEngine