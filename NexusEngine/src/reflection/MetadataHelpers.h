#pragma once

#include "NexusEngineApi.h"
#include "EntityReflection.h"

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

    NEXUS_ENGINE_API std::optional<ComponentView> CreateComponentView(TypeId type, void* componentData);

    NEXUS_ENGINE_API std::optional<ComponentView> CreateEntityComponentView(const ComponentMetadata& metadata, const flecs::entity& entity);

    template<typename T>
    std::optional<ComponentView> CreateComponentView(T& component)
    {
        return CreateComponentView(std::type_index(typeid(T)), &component);
    }

    NEXUS_ENGINE_API void ForEachField(const ComponentView& component, const std::function<void(const FieldView&)>& visitor);
    NEXUS_ENGINE_API std::optional<FieldView> FindField(const ComponentView& component, std::string_view fieldName);
    NEXUS_ENGINE_API FieldValueKind GetFieldValueKind(const FieldView& field);
    NEXUS_ENGINE_API std::string GetFieldTypeLabel(const FieldView& field);
    NEXUS_ENGINE_API std::optional<bool> ReadFieldBool(const FieldView& field);
    NEXUS_ENGINE_API std::optional<int> ReadFieldInt(const FieldView& field);
    NEXUS_ENGINE_API std::optional<float> ReadFieldFloat(const FieldView& field);
    NEXUS_ENGINE_API std::optional<std::string> ReadFieldText(const FieldView& field);
    NEXUS_ENGINE_API bool WriteFieldBool(const ComponentMetadata& metadata, const flecs::entity& entity, std::string_view fieldName, bool value);
    NEXUS_ENGINE_API bool WriteFieldInt(const ComponentMetadata& metadata, const flecs::entity& entity, std::string_view fieldName, int value);
    NEXUS_ENGINE_API bool WriteFieldFloat(const ComponentMetadata& metadata, const flecs::entity& entity, std::string_view fieldName, float value);
    NEXUS_ENGINE_API bool WriteFieldText(const ComponentMetadata& metadata, const flecs::entity& entity, std::string_view fieldName, std::string_view text);
    NEXUS_ENGINE_API bool IsFieldReadOnly(const FieldView& field);
} // namespace NexusEngine
