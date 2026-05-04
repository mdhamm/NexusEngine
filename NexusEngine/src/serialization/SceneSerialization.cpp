#include "SceneSerialization.h"
#include "EntitySerialization.h"
#include "ISerializer.h"
#include "components/EditorOnlyComponent.h"
#include "components/TransformComponent.h"

namespace NexusEngine
{
    void SerializeScene(const Scene& scene, ISerializeWriter& writer)
    {
        writer.Write("name", scene.m_name);
        writer.BeginArray("entities");
        scene.m_world.each<TransformComponent>(
            [&](flecs::entity entity, TransformComponent&)
            {
                if (!entity.is_valid() || !entity.is_alive() || entity.has<EditorOnlyComponent>())
                {
                    return;
                }

                writer.BeginArrayElement();
                SerializeEntity(entity, writer);
                writer.EndArrayElement();
            });
        writer.EndArray();
        writer.EndObject();
    }

} // namespace NexusEngine