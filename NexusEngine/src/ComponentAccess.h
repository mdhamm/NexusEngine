#pragma once

#include "MetadataRegistry.h"

#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace NexusEngine
{
    enum class FieldValueKind
    {
        Unsupported,
        Bool,
        Int,
        Float,
        String,
        AssetReference,
        NamedValue
    };

    std::optional<ComponentView> CreateComponentView(TypeId type, void* componentData);

    std::optional<ComponentView> CreateEntityComponentView(const ComponentMetadata& metadata, const flecs::entity& entity);

    template<typename T>
    std::optional<ComponentView> CreateComponentView(T& component)
    {
        return CreateComponentView(std::type_index(typeid(T)), &component);
    }

    void ForEachField(const ComponentView& component, const std::function<void(const FieldView&)>& visitor);
    std::optional<FieldView> FindField(const ComponentView& component, std::string_view fieldName);
    FieldValueKind GetFieldValueKind(const FieldView& field);
    std::string GetFieldTypeLabel(const FieldView& field);
    std::optional<bool> ReadFieldBool(const FieldView& field);
    std::optional<int> ReadFieldInt(const FieldView& field);
    std::optional<float> ReadFieldFloat(const FieldView& field);
    std::optional<std::string> ReadFieldText(const FieldView& field);
    bool WriteFieldBool(const ComponentMetadata& metadata, const flecs::entity& entity, std::string_view fieldName, bool value);
    bool WriteFieldInt(const ComponentMetadata& metadata, const flecs::entity& entity, std::string_view fieldName, int value);
    bool WriteFieldFloat(const ComponentMetadata& metadata, const flecs::entity& entity, std::string_view fieldName, float value);
    bool WriteFieldText(const ComponentMetadata& metadata, const flecs::entity& entity, std::string_view fieldName, std::string_view text);
    bool IsFieldReadOnly(const FieldView& field);
} // namespace NexusEngine
