#include "PropertyWidgetSerializer.h"

#include "EditorMaterialSerializer.h"
#include "EditorSceneSerializer.h"
#include "PropertyWidget.h"
#include "windows/EditorWindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QPushButton>
#include <QSignalBlocker>

#include <algorithm>

namespace NexusEditor
{
    namespace
    {
        QString MakePropertyControlObjectName(const std::string& componentName, const std::string& propertyName)
        {
            return QStringLiteral("PropertyWidget.%1.%2")
                .arg(QString::fromStdString(componentName), QString::fromStdString(propertyName));
        }

        QString GetInspectorFieldTypeLabel(const NexusEngine::FieldView& field)
        {
            return QString::fromStdString(NexusEngine::GetFieldTypeLabel(field));
        }

        class AssetReferenceDropLineEdit final : public QLineEdit
        {
        public:
            using QLineEdit::QLineEdit;

            std::function<bool(const QString&)> m_canAcceptAssetPath;
            std::function<bool(const QString&)> m_acceptAssetPath;

        protected:
            static QString GetDraggedAssetPath(const QMimeData* mimeData)
            {
                if (!mimeData)
                {
                    return {};
                }

                if (mimeData->hasUrls())
                {
                    const QList<QUrl> urls = mimeData->urls();
                    if (!urls.isEmpty())
                    {
                        const QString localFile = urls.front().toLocalFile().trimmed();
                        if (!localFile.isEmpty())
                        {
                            return localFile;
                        }
                    }
                }

                return mimeData->text().trimmed();
            }

            void dragEnterEvent(QDragEnterEvent* event) override
            {
                if (!event || !event->mimeData())
                {
                    QLineEdit::dragEnterEvent(event);
                    return;
                }

                const QString assetPath = GetDraggedAssetPath(event->mimeData());
                if (!assetPath.isEmpty() && m_canAcceptAssetPath && m_canAcceptAssetPath(assetPath))
                {
                    event->acceptProposedAction();
                    return;
                }

                QLineEdit::dragEnterEvent(event);
            }

            void dragMoveEvent(QDragMoveEvent* event) override
            {
                if (!event || !event->mimeData())
                {
                    QLineEdit::dragMoveEvent(event);
                    return;
                }

                const QString assetPath = GetDraggedAssetPath(event->mimeData());
                if (!assetPath.isEmpty() && m_canAcceptAssetPath && m_canAcceptAssetPath(assetPath))
                {
                    event->acceptProposedAction();
                    return;
                }

                QLineEdit::dragMoveEvent(event);
            }

            void dropEvent(QDropEvent* event) override
            {
                if (!event || !event->mimeData())
                {
                    QLineEdit::dropEvent(event);
                    return;
                }

                const QString assetPath = GetDraggedAssetPath(event->mimeData());
                if (!assetPath.isEmpty() && m_acceptAssetPath && m_acceptAssetPath(assetPath))
                {
                    event->acceptProposedAction();
                    return;
                }

                QLineEdit::dropEvent(event);
            }
        };

        class BoolFieldSerializer final : public IPropertyWidgetFieldSerializer
        {
        public:
            bool Supports(const NexusEngine::FieldView& field) const override
            {
                return NexusEngine::GetFieldValueKind(field) == NexusEngine::FieldValueKind::Bool;
            }

            QWidget* CreateEditor(
                const PropertyWidgetSerializerContext&,
                const flecs::entity& entity,
                const NexusEngine::ComponentMetadata& metadata,
                const NexusEngine::FieldView& field,
                QWidget* parent) const override
            {
                const std::string fieldName = field.m_meta ? field.m_meta->m_name : std::string{};
                auto* checkBox = new QCheckBox(parent);
                checkBox->setObjectName(MakePropertyControlObjectName(metadata.m_name, fieldName));
                checkBox->setChecked(NexusEngine::ReadFieldBool(field).value_or(false));
                checkBox->setEnabled(!NexusEngine::IsFieldReadOnly(field));
                if (!NexusEngine::IsFieldReadOnly(field))
                {
                    const NexusEngine::ComponentMetadata* metadataPtr = &metadata;
                    QObject::connect(checkBox, &QCheckBox::toggled, parent, [entity, metadataPtr, fieldName](bool checked)
                        {
                            (void)NexusEngine::WriteFieldBool(*metadataPtr, entity, fieldName, checked);
                        });
                }

                return checkBox;
            }

            void SyncEditor(const PropertyWidgetSerializerContext&, const NexusEngine::FieldView& field, QWidget* editor) const override
            {
                auto* checkBox = qobject_cast<QCheckBox*>(editor);
                if (!checkBox)
                {
                    return;
                }

                const bool isChecked = NexusEngine::ReadFieldBool(field).value_or(false);
                QSignalBlocker blocker(checkBox);
                checkBox->setChecked(isChecked);
                checkBox->setEnabled(!NexusEngine::IsFieldReadOnly(field));
            }
        };

        class NamedValueFieldSerializer final : public IPropertyWidgetFieldSerializer
        {
        public:
            bool Supports(const NexusEngine::FieldView& field) const override
            {
                return NexusEngine::GetFieldValueKind(field) == NexusEngine::FieldValueKind::NamedValue;
            }

            QWidget* CreateEditor(
                const PropertyWidgetSerializerContext&,
                const flecs::entity& entity,
                const NexusEngine::ComponentMetadata& metadata,
                const NexusEngine::FieldView& field,
                QWidget* parent) const override
            {
                const std::string fieldName = field.m_meta ? field.m_meta->m_name : std::string{};
                auto* comboBox = new QComboBox(parent);
                comboBox->setObjectName(MakePropertyControlObjectName(metadata.m_name, fieldName));
                for (const auto& namedValue : field.m_meta->m_namedValues)
                {
                    comboBox->addItem(QString::fromStdString(namedValue.first), namedValue.second);
                }

                const std::string selectedText = NexusEngine::ReadFieldText(field).value_or(std::string{});
                comboBox->setCurrentText(QString::fromStdString(selectedText));
                comboBox->setEnabled(!NexusEngine::IsFieldReadOnly(field));
                if (!NexusEngine::IsFieldReadOnly(field))
                {
                    const NexusEngine::ComponentMetadata* metadataPtr = &metadata;
                    QObject::connect(comboBox, &QComboBox::currentTextChanged, parent, [entity, metadataPtr, fieldName](const QString& text)
                        {
                            (void)NexusEngine::WriteFieldText(*metadataPtr, entity, fieldName, text.toStdString());
                        });
                }

                return comboBox;
            }

            void SyncEditor(const PropertyWidgetSerializerContext&, const NexusEngine::FieldView& field, QWidget* editor) const override
            {
                auto* comboBox = qobject_cast<QComboBox*>(editor);
                if (!comboBox)
                {
                    return;
                }

                const QString newValue = QString::fromStdString(NexusEngine::ReadFieldText(field).value_or(std::string{}));
                QSignalBlocker blocker(comboBox);
                comboBox->setCurrentText(newValue);
                comboBox->setEnabled(!NexusEngine::IsFieldReadOnly(field));
            }
        };

        class TextLineEditFieldSerializer final : public IPropertyWidgetFieldSerializer
        {
        public:
            explicit TextLineEditFieldSerializer(NexusEngine::FieldValueKind supportedKind)
                : m_supportedKind(supportedKind)
            {
            }

            bool Supports(const NexusEngine::FieldView& field) const override
            {
                return NexusEngine::GetFieldValueKind(field) == m_supportedKind;
            }

            QWidget* CreateEditor(
                const PropertyWidgetSerializerContext&,
                const flecs::entity& entity,
                const NexusEngine::ComponentMetadata& metadata,
                const NexusEngine::FieldView& field,
                QWidget* parent) const override
            {
                const std::string fieldName = field.m_meta ? field.m_meta->m_name : std::string{};
                auto* lineEdit = new QLineEdit(parent);
                lineEdit->setObjectName(MakePropertyControlObjectName(metadata.m_name, fieldName));
                lineEdit->setText(QString::fromStdString(NexusEngine::ReadFieldText(field).value_or(std::string{})));
                lineEdit->setReadOnly(NexusEngine::IsFieldReadOnly(field));
                if (!NexusEngine::IsFieldReadOnly(field))
                {
                    const NexusEngine::ComponentMetadata* metadataPtr = &metadata;
                    QObject::connect(lineEdit, &QLineEdit::editingFinished, parent, [entity, lineEdit, metadataPtr, fieldName]()
                        {
                            (void)NexusEngine::WriteFieldText(*metadataPtr, entity, fieldName, lineEdit->text().toStdString());
                        });
                }

                return lineEdit;
            }

            void SyncEditor(const PropertyWidgetSerializerContext&, const NexusEngine::FieldView& field, QWidget* editor) const override
            {
                auto* lineEdit = qobject_cast<QLineEdit*>(editor);
                if (!lineEdit)
                {
                    return;
                }

                const QString newValue = QString::fromStdString(NexusEngine::ReadFieldText(field).value_or(std::string{}));
                QSignalBlocker blocker(lineEdit);
                if (lineEdit->text() != newValue)
                {
                    lineEdit->setText(newValue);
                }
                lineEdit->setReadOnly(NexusEngine::IsFieldReadOnly(field));
            }

        private:
            NexusEngine::FieldValueKind m_supportedKind = NexusEngine::FieldValueKind::Unsupported;
        };

        class AssetReferenceFieldSerializer final : public IPropertyWidgetFieldSerializer
        {
        public:
            bool Supports(const NexusEngine::FieldView& field) const override
            {
                return NexusEngine::GetFieldValueKind(field) == NexusEngine::FieldValueKind::AssetReference && field.m_meta && field.m_meta->m_assetType.has_value();
            }

            QWidget* CreateEditor(
                const PropertyWidgetSerializerContext& context,
                const flecs::entity& entity,
                const NexusEngine::ComponentMetadata& metadata,
                const NexusEngine::FieldView& field,
                QWidget* parent) const override
            {
                const std::string fieldName = field.m_meta ? field.m_meta->m_name : std::string{};
                const QString objectName = MakePropertyControlObjectName(metadata.m_name, fieldName);
                PropertyWidget& propertyWidget = context.GetPropertyWidget();
                auto* container = new QWidget(parent);
                container->setObjectName(objectName);

                auto* layout = new QHBoxLayout(container);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->setSpacing(4);

                auto* lineEdit = new AssetReferenceDropLineEdit(container);
                lineEdit->setAcceptDrops(true);
                lineEdit->setText(propertyWidget.GetAssetReferenceDisplayPath(NexusEngine::ReadFieldText(field).value_or(std::string{})));
                lineEdit->setReadOnly(true);
                layout->addWidget(lineEdit, 1);

                auto* pickButton = new QPushButton(QStringLiteral("Pick"), container);
                pickButton->setObjectName(QStringLiteral("PickButton"));
                auto* clearButton = new QPushButton(QStringLiteral("Clear"), container);
                clearButton->setObjectName(QStringLiteral("ClearButton"));
                layout->addWidget(pickButton);
                layout->addWidget(clearButton);

                const NexusEngine::AssetType assetType = *field.m_meta->m_assetType;
                const NexusEngine::ComponentMetadata* metadataPtr = &metadata;

                auto assignAssetPath = [metadataPtr, entity, fieldName, lineEdit, &propertyWidget, assetType](const QString& assetPath)
                {
                    if (!propertyWidget.IsAcceptedAssetPath(assetPath, assetType))
                    {
                        return false;
                    }

                    if (!propertyWidget.AssignAssetReferencePath(*metadataPtr, entity, fieldName, assetPath))
                    {
                        return false;
                    }

                    lineEdit->setText(propertyWidget.NormalizeAssetPath(assetPath));
                    propertyWidget.ClearAssetReferencePick();
                    return true;
                };

                lineEdit->m_canAcceptAssetPath = [&propertyWidget, assetType](const QString& assetPath)
                {
                    return propertyWidget.IsAcceptedAssetPath(assetPath, assetType);
                };
                lineEdit->m_acceptAssetPath = assignAssetPath;

                const bool isReadOnly = NexusEngine::IsFieldReadOnly(field);
                pickButton->setEnabled(!isReadOnly);
                clearButton->setEnabled(!isReadOnly);
                if (!isReadOnly)
                {
                    QObject::connect(pickButton, &QPushButton::clicked, container, [&propertyWidget, objectName, metadata, fieldName, assetType]()
                        {
                            propertyWidget.BeginAssetReferencePick(metadata.m_name, fieldName, assetType, objectName);
                        });
                    QObject::connect(clearButton, &QPushButton::clicked, container, [metadataPtr, entity, fieldName, lineEdit, &propertyWidget]()
                        {
                            (void)NexusEngine::WriteFieldText(*metadataPtr, entity, fieldName, std::string_view{});
                            lineEdit->clear();
                            propertyWidget.ClearAssetReferencePick();
                        });
                }

                return container;
            }

            void SyncEditor(const PropertyWidgetSerializerContext& context, const NexusEngine::FieldView& field, QWidget* editor) const override
            {
                auto* container = qobject_cast<QWidget*>(editor);
                if (!container)
                {
                    return;
                }

                auto* lineEdit = container->findChild<QLineEdit*>();
                const QList<QPushButton*> buttons = container->findChildren<QPushButton*>(QString(), Qt::FindDirectChildrenOnly);
                if (!lineEdit)
                {
                    return;
                }

                PropertyWidget& propertyWidget = context.GetPropertyWidget();
                const QString newValue = propertyWidget.GetAssetReferenceDisplayPath(NexusEngine::ReadFieldText(field).value_or(std::string{}));
                QSignalBlocker blocker(lineEdit);
                if (lineEdit->text() != newValue)
                {
                    lineEdit->setText(newValue);
                }

                const bool isReadOnly = NexusEngine::IsFieldReadOnly(field);
                for (QPushButton* button : buttons)
                {
                    button->setEnabled(!isReadOnly);
                }

                container->setStyleSheet(propertyWidget.IsAssetReferencePickActive(container->objectName())
                    ? QStringLiteral("QWidget { border: 1px solid palette(highlight); border-radius: 3px; }")
                    : QString{});
            }
        };

        class UnsupportedFieldSerializer final : public IPropertyWidgetFieldSerializer
        {
        public:
            bool Supports(const NexusEngine::FieldView&) const override
            {
                return true;
            }

            QWidget* CreateEditor(
                const PropertyWidgetSerializerContext&,
                const flecs::entity&,
                const NexusEngine::ComponentMetadata& metadata,
                const NexusEngine::FieldView& field,
                QWidget* parent) const override
            {
                const std::string fieldName = field.m_meta ? field.m_meta->m_name : std::string{};
                auto* label = new QLabel(QStringLiteral("Unsupported"), parent);
                label->setObjectName(MakePropertyControlObjectName(metadata.m_name, fieldName));
                return label;
            }

            void SyncEditor(const PropertyWidgetSerializerContext&, const NexusEngine::FieldView&, QWidget*) const override
            {
            }
        };
    }

    PropertyWidgetSerializer::PropertyWidgetSerializer(PropertyWidget& propertyWidget)
        : m_context(propertyWidget)
    {
        m_serializers.push_back(std::make_unique<BoolFieldSerializer>());
        m_serializers.push_back(std::make_unique<NamedValueFieldSerializer>());
        m_serializers.push_back(std::make_unique<AssetReferenceFieldSerializer>());
        m_serializers.push_back(std::make_unique<TextLineEditFieldSerializer>(NexusEngine::FieldValueKind::Int));
        m_serializers.push_back(std::make_unique<TextLineEditFieldSerializer>(NexusEngine::FieldValueKind::Float));
        m_serializers.push_back(std::make_unique<TextLineEditFieldSerializer>(NexusEngine::FieldValueKind::String));
        m_serializers.push_back(std::make_unique<UnsupportedFieldSerializer>());
    }

    QWidget* PropertyWidgetSerializer::CreateFieldEditor(
        const flecs::entity& entity,
        const NexusEngine::ComponentMetadata& metadata,
        const NexusEngine::FieldView& field,
        QWidget* parent) const
    {
        const IPropertyWidgetFieldSerializer* serializer = FindSerializer(field);
        return serializer ? serializer->CreateEditor(m_context, entity, metadata, field, parent) : nullptr;
    }

    void PropertyWidgetSerializer::SyncFieldEditor(const NexusEngine::FieldView& field, QWidget* editor) const
    {
        const IPropertyWidgetFieldSerializer* serializer = FindSerializer(field);
        if (serializer)
        {
            serializer->SyncEditor(m_context, field, editor);
        }
    }

    const IPropertyWidgetFieldSerializer* PropertyWidgetSerializer::FindSerializer(const NexusEngine::FieldView& field) const
    {
        const auto it = std::find_if(m_serializers.begin(), m_serializers.end(), [&](const FieldSerializerPtr& serializer)
        {
            return serializer && serializer->Supports(field);
        });
        return it != m_serializers.end() ? it->get() : nullptr;
    }

    void AddPropertyWidgetFieldRow(
        const PropertyWidgetSerializer& serializer,
        QFormLayout* formLayout,
        QGroupBox* groupBox,
        const flecs::entity& entity,
        const NexusEngine::ComponentMetadata& metadata,
        const NexusEngine::FieldView& field)
    {
        if (!formLayout || !groupBox)
        {
            return;
        }

        const std::string fieldName = field.m_meta ? field.m_meta->m_name : std::string{};
        const QString labelText = QStringLiteral("%1 (%2)")
            .arg(QString::fromStdString(fieldName), GetInspectorFieldTypeLabel(field));
        if (QWidget* editor = serializer.CreateFieldEditor(entity, metadata, field, groupBox))
        {
            formLayout->addRow(labelText, editor);
        }
        else
        {
            formLayout->addRow(labelText, new QLabel(QStringLiteral("Unsupported"), groupBox));
        }
    }
} // namespace NexusEditor
