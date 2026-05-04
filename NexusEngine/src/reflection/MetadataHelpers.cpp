#include "MetadataHelpers.h"

#include <algorithm>
#include <sstream>

namespace NexusEngine
{
    namespace
    {
        std::string FormatFieldFloat(float value)
        {
            std::ostringstream stream;
            stream.precision(4);
            stream << std::fixed << value;
            return stream.str();
        }

        bool ApplyAfterWrite(const ComponentMetadata& metadata, const flecs::entity& entity, void* componentData)
        {
            if (metadata.m_afterApply)
            {
                metadata.m_afterApply(entity, componentData);
            }

            return true;
        }

        std::optional<FieldView> FindMutableField(const ComponentMetadata& metadata, const flecs::entity& entity, std::string_view fieldName, ComponentView& componentView)
        {
            const std::optional<ComponentView> view = CreateEntityComponentView(metadata, entity);
            if (!view)
            {
                return std::nullopt;
            }

            componentView = *view;
            return FindField(componentView, fieldName);
        }
    }

    std::optional<ComponentView> CreateComponentView(TypeId type, void* componentData)
    {
        const ComponentMetadata* metadata = MetadataRegistry::Instance().Get(type);
        if (!metadata || !componentData)
        {
            return std::nullopt;
        }

        ComponentView view;
        view.m_data = componentData;
        view.m_meta = metadata;
        return view;
    }

    std::optional<ComponentView> CreateEntityComponentView(const ComponentMetadata& metadata, const flecs::entity& entity)
    {
        if (!metadata.m_hasComponent || !metadata.m_hasComponent(entity) || !metadata.m_getMutableComponent)
        {
            return std::nullopt;
        }

        void* componentData = metadata.m_getMutableComponent(entity);
        if (!componentData)
        {
            return std::nullopt;
        }

        return CreateComponentView(metadata.m_componentType, componentData);
    }

    void ForEachField(const ComponentView& component, const std::function<void(const FieldView&)>& visitor)
    {
        if (!component.m_meta || !component.m_data)
        {
            return;
        }

        for (const FieldMetadata& fieldMetadata : component.m_meta->m_fields)
        {
            FieldView fieldView;
            fieldView.m_data = fieldMetadata.GetPointer(component.m_data);
            fieldView.m_meta = &fieldMetadata;
            visitor(fieldView);
        }
    }

    std::optional<FieldView> FindField(const ComponentView& component, std::string_view fieldName)
    {
        std::optional<FieldView> result;
        ForEachField(
            component,
            [&](const FieldView& field)
            {
                if (!result && field.m_meta && field.m_meta->m_name == fieldName)
                {
                    result = field;
                }
            });
        return result;
    }

    FieldValueKind GetFieldValueKind(const FieldView& field)
    {
        if (!field.m_meta || !field.m_data)
        {
            return FieldValueKind::Unsupported;
        }

        if (!field.m_meta->m_namedValues.empty() && field.m_meta->m_size == sizeof(int))
        {
            return FieldValueKind::NamedValue;
        }

        if (field.m_meta->m_type == std::type_index(typeid(bool)))
        {
            return FieldValueKind::Bool;
        }

        if (field.m_meta->m_type == std::type_index(typeid(int)))
        {
            return FieldValueKind::Int;
        }

        if (field.m_meta->m_type == std::type_index(typeid(float)))
        {
            return FieldValueKind::Float;
        }

        if (field.m_meta->m_type == std::type_index(typeid(std::string)))
        {
            return field.m_meta->m_assetType.has_value()
                ? FieldValueKind::AssetReference
                : FieldValueKind::String;
        }

        return FieldValueKind::Unsupported;
    }

    std::string GetFieldTypeLabel(const FieldView& field)
    {
        switch (GetFieldValueKind(field))
        {
        case FieldValueKind::Bool:
            return "bool";
        case FieldValueKind::Int:
            return "int";
        case FieldValueKind::Float:
            return "float";
        case FieldValueKind::String:
            return "string";
        case FieldValueKind::AssetReference:
            return "AssetReference";
        case FieldValueKind::NamedValue:
            return "enum";
        default:
            return "unsupported";
        }
    }

    std::optional<bool> ReadFieldBool(const FieldView& field)
    {
        return GetFieldValueKind(field) == FieldValueKind::Bool
            ? std::optional<bool>(*static_cast<const bool*>(field.m_data))
            : std::nullopt;
    }

    std::optional<int> ReadFieldInt(const FieldView& field)
    {
        return GetFieldValueKind(field) == FieldValueKind::Int
            ? std::optional<int>(*static_cast<const int*>(field.m_data))
            : std::nullopt;
    }

    std::optional<float> ReadFieldFloat(const FieldView& field)
    {
        return GetFieldValueKind(field) == FieldValueKind::Float
            ? std::optional<float>(*static_cast<const float*>(field.m_data))
            : std::nullopt;
    }

    std::optional<std::string> ReadFieldText(const FieldView& field)
    {
        switch (GetFieldValueKind(field))
        {
        case FieldValueKind::String:
        case FieldValueKind::AssetReference:
            return *static_cast<const std::string*>(field.m_data);
        case FieldValueKind::NamedValue:
        {
            const int value = *static_cast<const int*>(field.m_data);
            for (const auto& namedValue : field.m_meta->m_namedValues)
            {
                if (namedValue.second == value)
                {
                    return namedValue.first;
                }
            }
            return std::to_string(value);
        }
        case FieldValueKind::Int:
            return std::to_string(*static_cast<const int*>(field.m_data));
        case FieldValueKind::Float:
            return FormatFieldFloat(*static_cast<const float*>(field.m_data));
        case FieldValueKind::Bool:
            return *static_cast<const bool*>(field.m_data) ? std::string("true") : std::string("false");
        default:
            return std::nullopt;
        }
    }

    bool WriteFieldBool(const ComponentMetadata& metadata, const flecs::entity& entity, std::string_view fieldName, bool value)
    {
        ComponentView componentView;
        const std::optional<FieldView> field = FindMutableField(metadata, entity, fieldName, componentView);
        if (!field || GetFieldValueKind(*field) != FieldValueKind::Bool)
        {
            return false;
        }

        *static_cast<bool*>(field->m_data) = value;
        return ApplyAfterWrite(metadata, entity, componentView.m_data);
    }

    bool WriteFieldInt(const ComponentMetadata& metadata, const flecs::entity& entity, std::string_view fieldName, int value)
    {
        ComponentView componentView;
        const std::optional<FieldView> field = FindMutableField(metadata, entity, fieldName, componentView);
        if (!field)
        {
            return false;
        }

        if (GetFieldValueKind(*field) == FieldValueKind::Int || GetFieldValueKind(*field) == FieldValueKind::NamedValue)
        {
            *static_cast<int*>(field->m_data) = value;
            return ApplyAfterWrite(metadata, entity, componentView.m_data);
        }

        return false;
    }

    bool WriteFieldFloat(const ComponentMetadata& metadata, const flecs::entity& entity, std::string_view fieldName, float value)
    {
        ComponentView componentView;
        const std::optional<FieldView> field = FindMutableField(metadata, entity, fieldName, componentView);
        if (!field || GetFieldValueKind(*field) != FieldValueKind::Float)
        {
            return false;
        }

        *static_cast<float*>(field->m_data) = value;
        return ApplyAfterWrite(metadata, entity, componentView.m_data);
    }

    bool WriteFieldText(const ComponentMetadata& metadata, const flecs::entity& entity, std::string_view fieldName, std::string_view text)
    {
        ComponentView componentView;
        const std::optional<FieldView> field = FindMutableField(metadata, entity, fieldName, componentView);
        if (!field)
        {
            return false;
        }

        switch (GetFieldValueKind(*field))
        {
        case FieldValueKind::String:
        case FieldValueKind::AssetReference:
            *static_cast<std::string*>(field->m_data) = std::string(text);
            return ApplyAfterWrite(metadata, entity, componentView.m_data);
        case FieldValueKind::NamedValue:
        {
            const auto it = field->m_meta->m_namedValues.find(std::string(text));
            if (it == field->m_meta->m_namedValues.end())
            {
                return false;
            }

            *static_cast<int*>(field->m_data) = it->second;
            return ApplyAfterWrite(metadata, entity, componentView.m_data);
        }
        case FieldValueKind::Int:
            try
            {
                *static_cast<int*>(field->m_data) = std::stoi(std::string(text));
                return ApplyAfterWrite(metadata, entity, componentView.m_data);
            }
            catch (...)
            {
                return false;
            }
        case FieldValueKind::Float:
            try
            {
                *static_cast<float*>(field->m_data) = std::stof(std::string(text));
                return ApplyAfterWrite(metadata, entity, componentView.m_data);
            }
            catch (...)
            {
                return false;
            }
        case FieldValueKind::Bool:
            if (text == "true")
            {
                *static_cast<bool*>(field->m_data) = true;
                return ApplyAfterWrite(metadata, entity, componentView.m_data);
            }
            if (text == "false")
            {
                *static_cast<bool*>(field->m_data) = false;
                return ApplyAfterWrite(metadata, entity, componentView.m_data);
            }
            return false;
        default:
            return false;
        }
    }

    bool IsFieldReadOnly(const FieldView& field)
    {
        return GetFieldValueKind(field) == FieldValueKind::Unsupported;
    }
} // namespace NexusEngine
