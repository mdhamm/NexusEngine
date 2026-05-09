#pragma once

#include <QWidget>

#ifdef emit
#undef emit
#endif

#include <reflection/EntityReflection.h>
#include <reflection/MetadataHelpers.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

class QFormLayout;
class QGroupBox;

namespace NexusEditor
{
    class PropertyWidget;

    class PropertyWidgetSerializerContext
    {
    public:
        explicit PropertyWidgetSerializerContext(PropertyWidget& propertyWidget)
            : m_propertyWidget(&propertyWidget)
        {
        }

        PropertyWidget& GetPropertyWidget() const { return *m_propertyWidget; }

    private:
        PropertyWidget* m_propertyWidget = nullptr;
    };

    class IPropertyWidgetFieldSerializer
    {
    public:
        virtual ~IPropertyWidgetFieldSerializer() = default;

        virtual bool Supports(const NexusEngine::FieldView& field) const = 0;
        virtual QWidget* CreateEditor(
            const PropertyWidgetSerializerContext& context,
            const flecs::entity& entity,
            const NexusEngine::ComponentMetadata& metadata,
            const NexusEngine::FieldView& field,
            QWidget* parent) const = 0;
        virtual void SyncEditor(
            const PropertyWidgetSerializerContext& context,
            const NexusEngine::FieldView& field,
            QWidget* editor) const = 0;
    };

    class PropertyWidgetSerializer final
    {
    public:
        explicit PropertyWidgetSerializer(PropertyWidget& propertyWidget);

        QWidget* CreateFieldEditor(
            const flecs::entity& entity,
            const NexusEngine::ComponentMetadata& metadata,
            const NexusEngine::FieldView& field,
            QWidget* parent) const;
        void SyncFieldEditor(const NexusEngine::FieldView& field, QWidget* editor) const;

    private:
        using FieldSerializerPtr = std::unique_ptr<IPropertyWidgetFieldSerializer>;

        const IPropertyWidgetFieldSerializer* FindSerializer(const NexusEngine::FieldView& field) const;

        PropertyWidgetSerializerContext m_context;
        std::vector<FieldSerializerPtr> m_serializers;
    };

    void AddPropertyWidgetFieldRow(
        const PropertyWidgetSerializer& serializer,
        QFormLayout* formLayout,
        QGroupBox* groupBox,
        const flecs::entity& entity,
        const NexusEngine::ComponentMetadata& metadata,
        const NexusEngine::FieldView& field);
} // namespace NexusEditor
