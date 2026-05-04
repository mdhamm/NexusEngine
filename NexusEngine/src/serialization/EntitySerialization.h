#pragma once

#include "ISerializer.h"
#include "MetadataRegistry.h"
#include "Scene.h"
// TODO: move to cpp

namespace NexusEngine
{
    void SerializeField(const FieldMetadata& fieldMeta, const void* data, ISerializer& s)
    {
        if (fieldMeta.runtimeType == GetTypeId<float>())
        {
            s.Write(fieldMeta.name, *static_cast<const float*>(data));
        }
        else if (fieldMeta.runtimeType == GetTypeId<bool>())
        {
            s.Write(fieldMeta.name, *static_cast<const bool*>(data));
        }
        else if (fieldMeta.runtimeType == GetTypeId<int>())
        {
            s.Write(fieldMeta.name, *static_cast<const int*>(data));
        }
        else if (fieldMeta.runtimeType == GetTypeId<std::string>())
        {
            s.Write(fieldMeta.name, *static_cast<const std::string*>(data));
        }
        else if (fieldMeta.runtimeType == GetTypeId<AssetReference>())
        {
            const auto& ref = *static_cast<const AssetReference*>(data);
            s.Write(fieldMeta.name, ref.m_guid);
        }
    }

    void SerializeComponent(const ComponentMetadata& component, ISerializer& s)
    {
        const ComponentMetadata& meta = component.meta;

        s.BeginObject(meta.name);

        for (const FieldMetadata& field : meta.fields)
        {
            const void* fieldPtr =
                static_cast<const char*>(component.data) + field.offset;

            SerializeField(field, fieldPtr, s);
        }

        s.EndObject();
    }

    void SerializeEntity(flecs::entity e, ISerializeWriter& writer)
    {
        writer.BeginObject();

        writer.Write("id", e.id());
        writer.Write("name", scene.GetName(e));

        const std::uint64_t parentId =
            entity.parent().is_valid()
            ? static_cast<std::uint64_t>(entity.parent().id())
            : 0;

        writer.Write("parentId", parentId);

        writer.BeginArray("components");

        for (const auto& [type, metadata] : MetadataRegistry::Instance().GetAll())
        {
            (void)type;

            if (!metadata.m_hasComponent || !metadata.m_hasComponent(entity))
            {
                continue;
            }

            SerializeComponent(metadata, entity, writer);
        }

        writer.EndArray(); // components

        writer.EndObject(); // entity
    }
} // namespace NexusEngine