#pragma once

#include "reflection/EntityReflection.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace NexusEngine
{
    class ISerializeReader;
    class ISerializeWriter;

    struct SerializedEntityHeader
    {
        std::uint64_t m_id = 0;
        std::string m_name;
        std::uint64_t m_parentId = 0;
    };

    bool HasObjectKey(const ISerializeReader& reader, std::string_view name);

    void SerializeField(const FieldView& field, ISerializeWriter& writer);

    void SerializeComponent(const ComponentMetadata& metadata, const flecs::entity& entity, ISerializeWriter& writer);

    void SerializeEntity(const flecs::entity& entity, ISerializeWriter& writer);

    bool DeserializeField(std::string& fieldName, std::string& fieldValue, ISerializeReader& reader);

    bool DeserializeEntityHeader(SerializedEntityHeader& header, ISerializeReader& reader);

    bool DeserializeComponent(const flecs::entity& entity, ISerializeReader& reader);

    bool DeserializeEntity(const flecs::entity& entity, ISerializeReader& reader);
} // namespace NexusEngine