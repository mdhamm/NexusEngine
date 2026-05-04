#include "EntitySerialization.h"
#include "ComponentAccess.h"
#include "ISerializer.h"

#include <algorithm>
#include <optional>
#include <vector>

namespace NexusEngine
{
    bool HasObjectKey(const ISerializeReader& reader, std::string_view name)
    {
        const std::string key(name);
        const std::vector<std::string> keys = reader.GetObjectKeys();
        return std::find(keys.begin(), keys.end(), key) != keys.end();
    }

    void SerializeField(const FieldView& field, ISerializeWriter& writer)
    {
        if (!field.m_meta || !field.m_data)
        {
            return;
        }

        writer.Write("name", field.m_meta->m_name);
        writer.Write("type", GetFieldTypeLabel(field));
        writer.Write("value", ReadFieldText(field).value_or(std::string{}));
        writer.Write("readOnly", IsFieldReadOnly(field));
    }

    void SerializeComponent(const ComponentMetadata& metadata, const flecs::entity& entity, ISerializeWriter& writer)
    {
        writer.Write("type", "Component");
        writer.Write("typeId", metadata.m_name);
        writer.BeginArray("fields");

        const std::optional<ComponentView> componentView = CreateEntityComponentView(metadata, entity);
        if (componentView)
        {
            ForEachField(
                *componentView,
                [&](const FieldView& field)
                {
                    writer.BeginArrayElement();
                    SerializeField(field, writer);
                    writer.EndArrayElement();
                });
        }

        writer.EndArray();
    }

    void SerializeEntity(const flecs::entity& entity, ISerializeWriter& writer)
    {
        writer.Write("id", static_cast<std::uint64_t>(entity.id()));
        writer.Write("name", entity.name() ? std::string_view(entity.name()) : std::string_view{});

        const flecs::entity parent = entity.parent();
        writer.Write(
            "parentId",
            parent.is_valid()
                ? static_cast<std::uint64_t>(parent.id())
                : static_cast<std::uint64_t>(0));

        writer.BeginArray("components");

        for (const auto& [type, metadata] : MetadataRegistry::Instance().GetAll())
        {
            (void)type;
            if (!metadata.m_hasComponent || !metadata.m_hasComponent(entity))
            {
                continue;
            }

            writer.BeginArrayElement();
            SerializeComponent(metadata, entity, writer);
            writer.EndArrayElement();
        }

        writer.EndArray();
    }

    bool DeserializeField(std::string& fieldName, std::string& fieldValue, ISerializeReader& reader)
    {
        reader.Read("name", fieldName);
        reader.Read("value", fieldValue);
        return !fieldName.empty();
    }

    bool DeserializeEntityHeader(SerializedEntityHeader& header, ISerializeReader& reader)
    {
        header = {};
        reader.Read("id", header.m_id);
        reader.Read("name", header.m_name);
        reader.Read("parentId", header.m_parentId);
        return header.m_id != 0;
    }

    bool DeserializeComponent(const flecs::entity& entity, ISerializeReader& reader)
    {
        std::string typeId;
        reader.Read("typeId", typeId);
        if (typeId.empty())
        {
            return false;
        }

        const ComponentMetadata* metadata = MetadataRegistry::Instance().FindByName(typeId);
        if (!metadata)
        {
            return false;
        }

        if (metadata->m_addComponent && (!metadata->m_hasComponent || !metadata->m_hasComponent(entity)))
        {
            metadata->m_addComponent(entity);
        }

        const std::string_view fieldArrayName = HasObjectKey(reader, "fields")
            ? std::string_view("fields")
            : (HasObjectKey(reader, "properties") ? std::string_view("properties") : std::string_view{});
        if (fieldArrayName.empty())
        {
            return true;
        }

        const std::size_t fieldCount = reader.GetArraySize(fieldArrayName);
        reader.BeginArray(fieldArrayName);
        for (std::size_t fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex)
        {
            reader.BeginArrayElement(fieldIndex);

            std::string fieldName;
            std::string fieldValue;
            if (DeserializeField(fieldName, fieldValue, reader))
            {
                (void)WriteFieldText(*metadata, entity, fieldName, fieldValue);
            }

            reader.EndArrayElement();
        }
        reader.EndArray();

        return true;
    }

    bool DeserializeEntity(const flecs::entity& entity, ISerializeReader& reader)
    {
        if (!HasObjectKey(reader, "components"))
        {
            return true;
        }

        const std::size_t componentCount = reader.GetArraySize("components");
        reader.BeginArray("components");
        for (std::size_t componentIndex = 0; componentIndex < componentCount; ++componentIndex)
        {
            reader.BeginArrayElement(componentIndex);
            (void)DeserializeComponent(entity, reader);
            reader.EndArrayElement();
        }
        reader.EndArray();

        return true;
    }
} // namespace NexusEngine
