#include "PropertyWidget.h"

#include "EditorMaterialSerializer.h"
#include "EditorWindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QString>
#include <QVBoxLayout>

#ifdef emit
#undef emit
#endif

#include <ComponentAccess.h>
#include <MetadataRegistry.h>
#include <Scene.h>
#include <components/EditorOnlyComponent.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace NexusEditor
{
    namespace
    {
        constexpr const char* EntityNameLabelObjectName = "PropertyWidget.EntityName";

        void ClearLayout(QLayout* layout)
        {
            while (QLayoutItem* item = layout->takeAt(0))
            {
                if (QWidget* widget = item->widget())
                {
                    widget->deleteLater();
                }

                if (QLayout* childLayout = item->layout())
                {
                    ClearLayout(childLayout);
                    delete childLayout;
                }

                delete item;
            }
        }

        QString MakePropertyControlObjectName(const std::string& componentName, const std::string& propertyName)
        {
            return QStringLiteral("PropertyWidget.%1.%2")
                .arg(QString::fromStdString(componentName), QString::fromStdString(propertyName));
        }

        QString GetInspectorFieldTypeLabel(const NexusEngine::FieldView& field)
        {
            return QString::fromStdString(NexusEngine::GetFieldTypeLabel(field));
        }
    }

    PropertyWidget::PropertyWidget(EditorWindow& editorWindow, QWidget* parent)
        : QWidget(parent)
        , m_editorWindow(&editorWindow)
    {
        auto* rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(0, 0, 0, 0);

        auto* addComponentLayout = new QHBoxLayout();
        m_addComponentComboBox = new QComboBox(this);
        m_addComponentButton = new QPushButton(QStringLiteral("Add Component"), this);
        addComponentLayout->addWidget(m_addComponentComboBox, 1);
        addComponentLayout->addWidget(m_addComponentButton);
        rootLayout->addLayout(addComponentLayout);

        m_contentLayout = new QVBoxLayout();
        rootLayout->addLayout(m_contentLayout);
        rootLayout->addStretch(1);

        connect(m_addComponentButton, &QPushButton::clicked, this, [this]()
            {
                if (!m_editorWindow || !m_editorWindow->IsEngineInitialized() || m_selectedEntityId == 0)
                {
                    return;
                }

                NexusEngine::Scene* activeScene = m_editorWindow->GetActiveScene();
                if (!activeScene)
                {
                    return;
                }

                const QVariant selectedName = m_addComponentComboBox->currentData();
                if (!selectedName.isValid())
                {
                    return;
                }

                const std::string componentName = selectedName.toString().toStdString();
                const flecs::entity entity = activeScene->m_world.entity(static_cast<flecs::entity_t>(m_selectedEntityId));
                if (!entity.is_valid())
                {
                    return;
                }

                for (const auto& [type, metadata] : NexusEngine::MetadataRegistry::Instance().GetAll())
                {
                    (void)type;
                    if (metadata.m_name == componentName && metadata.m_addComponent)
                    {
                        metadata.m_addComponent(entity);
                        break;
                    }
                }

                Refresh();
            });

        Refresh();
    }

    void PropertyWidget::SetSelectedEntityId(std::uint64_t entityId)
    {
        m_selectedAssetPath.clear();
        if (m_selectedEntityId == entityId)
        {
            return;
        }

        m_selectedEntityId = entityId;
        Refresh();
    }

    void PropertyWidget::SetSelectedAssetPath(const QString& assetPath)
    {
        const QString cleanAssetPath = assetPath.trimmed();
        if (m_selectedAssetPath == cleanAssetPath)
        {
            return;
        }

        m_selectedAssetPath = cleanAssetPath;
        m_selectedEntityId = 0;
        Refresh();
    }

    void PropertyWidget::Refresh()
    {
        const bool isMaterialAssetSelected = IsMaterialAssetFilePath(m_selectedAssetPath);
        m_addComponentComboBox->setVisible(!isMaterialAssetSelected);
        if (m_addComponentButton)
        {
            m_addComponentButton->setVisible(!isMaterialAssetSelected);
        }

        if (isMaterialAssetSelected)
        {
            m_addComponentComboBox->clear();
            RebuildAssetContents();
            m_lastStructureSignature = QStringLiteral("asset:%1").arg(m_selectedAssetPath);
            return;
        }

        RebuildAddComponentOptions();
        RebuildContents();
        m_lastStructureSignature = CaptureStructureSignature();
    }

    void PropertyWidget::RefreshIfNotInteracting()
    {
        if (IsInteracting())
        {
            return;
        }

        if (IsMaterialAssetFilePath(m_selectedAssetPath))
        {
            return;
        }

        const QString currentStructureSignature = CaptureStructureSignature();
        if (currentStructureSignature == m_lastStructureSignature)
        {
            SyncDisplayedValues();
            return;
        }

        Refresh();
    }

    bool PropertyWidget::IsInteracting() const
    {
        QWidget* focusedWidget = QApplication::focusWidget();
        return focusedWidget && (focusedWidget == this || isAncestorOf(focusedWidget));
    }

    QString PropertyWidget::CaptureStructureSignature() const
    {
        if (!m_editorWindow || !m_editorWindow->IsEngineInitialized())
        {
            return QStringLiteral("state:initializing");
        }

        NexusEngine::Scene* activeScene = m_editorWindow->GetActiveScene();
        if (!activeScene)
        {
            return QStringLiteral("state:no-scene");
        }

        if (m_selectedEntityId == 0)
        {
            return QStringLiteral("state:no-selection");
        }

        const flecs::entity entity = activeScene->m_world.entity(static_cast<flecs::entity_t>(m_selectedEntityId));
        if (!entity.is_valid() || !entity.is_alive())
        {
            return QStringLiteral("state:invalid-selection");
        }

        QString signature = QStringLiteral("entity:%1")
            .arg(static_cast<qulonglong>(m_selectedEntityId));

        for (const auto& [type, metadata] : NexusEngine::MetadataRegistry::Instance().GetAll())
        {
            (void)type;
            if (!metadata.m_hasComponent || !metadata.m_hasComponent(entity))
            {
                continue;
            }

            signature += QStringLiteral("|component:%1").arg(QString::fromStdString(metadata.m_name));

            const std::optional<NexusEngine::ComponentView> componentView = NexusEngine::CreateEntityComponentView(metadata, entity);
            if (!componentView)
            {
                continue;
            }

            NexusEngine::ForEachField(
                *componentView,
                [&](const NexusEngine::FieldView& field)
                {
                    signature += QStringLiteral("|property:%1:%2:%3:%4")
                        .arg(QString::fromStdString(std::string(field.m_name)))
                        .arg(GetInspectorFieldTypeLabel(field))
                        .arg(static_cast<int>(NexusEngine::GetFieldValueKind(field)))
                        .arg(NexusEngine::IsFieldReadOnly(field) ? 1 : 0);
                });
        }

        signature += QStringLiteral("|addable:");
        for (const auto& [type, metadata] : NexusEngine::MetadataRegistry::Instance().GetAll())
        {
            (void)type;
            if (!metadata.m_addComponent || (metadata.m_hasComponent && metadata.m_hasComponent(entity)))
            {
                continue;
            }

            signature += QString::fromStdString(metadata.m_name);
            signature += QLatin1Char(';');
        }

        return signature;
    }

    void PropertyWidget::RebuildAssetContents()
    {
        ClearLayout(m_contentLayout);

        MaterialAssetData materialData;
        if (!LoadMaterialAssetFile(m_selectedAssetPath, materialData))
        {
            m_contentLayout->addWidget(new QLabel(QStringLiteral("Select an entity to inspect"), this));
            return;
        }

        auto* assetNameLabel = new QLabel(materialData.m_name, this);
        m_contentLayout->addWidget(assetNameLabel);

        auto* groupBox = new QGroupBox(QStringLiteral("Material"), this);
        auto* formLayout = new QFormLayout(groupBox);

        auto saveMaterial = [this](const MaterialAssetData& updatedMaterialData)
        {
            (void)SaveMaterialAssetFile(m_selectedAssetPath, updatedMaterialData);
        };

        auto* vertexShaderLineEdit = new QLineEdit(materialData.m_vertexShaderPath, groupBox);
        connect(vertexShaderLineEdit, &QLineEdit::editingFinished, groupBox, [saveMaterial, materialData, vertexShaderLineEdit]() mutable
            {
                materialData.m_vertexShaderPath = vertexShaderLineEdit->text().trimmed();
                saveMaterial(materialData);
            });
        formLayout->addRow(QStringLiteral("Vertex Shader (string)"), vertexShaderLineEdit);

        auto* pixelShaderLineEdit = new QLineEdit(materialData.m_pixelShaderPath, groupBox);
        connect(pixelShaderLineEdit, &QLineEdit::editingFinished, groupBox, [saveMaterial, materialData, pixelShaderLineEdit]() mutable
            {
                materialData.m_pixelShaderPath = pixelShaderLineEdit->text().trimmed();
                saveMaterial(materialData);
            });
        formLayout->addRow(QStringLiteral("Pixel Shader (string)"), pixelShaderLineEdit);

        auto* transparentCheckBox = new QCheckBox(groupBox);
        transparentCheckBox->setChecked(materialData.m_isTransparent);
        connect(transparentCheckBox, &QCheckBox::toggled, groupBox, [saveMaterial, materialData](bool checked) mutable
            {
                materialData.m_isTransparent = checked;
                saveMaterial(materialData);
            });
        formLayout->addRow(QStringLiteral("Transparent (bool)"), transparentCheckBox);

        auto* cullModeComboBox = new QComboBox(groupBox);
        cullModeComboBox->addItem(QStringLiteral("None"));
        cullModeComboBox->addItem(QStringLiteral("Front"));
        cullModeComboBox->addItem(QStringLiteral("Back"));
        cullModeComboBox->setCurrentText(materialData.m_cullMode);
        connect(cullModeComboBox, &QComboBox::currentTextChanged, groupBox, [saveMaterial, materialData](const QString& text) mutable
            {
                materialData.m_cullMode = text;
                saveMaterial(materialData);
            });
        formLayout->addRow(QStringLiteral("Cull Mode (string)"), cullModeComboBox);

        auto* depthTestCheckBox = new QCheckBox(groupBox);
        depthTestCheckBox->setChecked(materialData.m_depthTestEnabled);
        connect(depthTestCheckBox, &QCheckBox::toggled, groupBox, [saveMaterial, materialData](bool checked) mutable
            {
                materialData.m_depthTestEnabled = checked;
                saveMaterial(materialData);
            });
        formLayout->addRow(QStringLiteral("Depth Test Enabled (bool)"), depthTestCheckBox);

        auto* depthWriteCheckBox = new QCheckBox(groupBox);
        depthWriteCheckBox->setChecked(materialData.m_depthWriteEnabled);
        connect(depthWriteCheckBox, &QCheckBox::toggled, groupBox, [saveMaterial, materialData](bool checked) mutable
            {
                materialData.m_depthWriteEnabled = checked;
                saveMaterial(materialData);
            });
        formLayout->addRow(QStringLiteral("Depth Write Enabled (bool)"), depthWriteCheckBox);

        m_contentLayout->addWidget(groupBox);
    }

    void PropertyWidget::SyncDisplayedValues()
    {
        if (!m_editorWindow || !m_editorWindow->IsEngineInitialized())
        {
            return;
        }

        NexusEngine::Scene* activeScene = m_editorWindow->GetActiveScene();
        if (!activeScene || m_selectedEntityId == 0)
        {
            return;
        }

        const flecs::entity entity = activeScene->m_world.entity(static_cast<flecs::entity_t>(m_selectedEntityId));
        if (!entity.is_valid() || !entity.is_alive())
        {
            return;
        }

        if (entity.has<NexusEngine::EditorOnlyComponent>())
        {
            return;
        }

        if (auto* entityNameLabel = findChild<QLabel*>(QString::fromUtf8(EntityNameLabelObjectName)))
        {
            const char* entityName = entity.name();
            const QString newEntityLabel = entityName && entityName[0] != '\0'
                ? QString::fromUtf8(entityName)
                : QStringLiteral("Entity %1").arg(static_cast<qulonglong>(m_selectedEntityId));
            if (entityNameLabel->text() != newEntityLabel)
            {
                entityNameLabel->setText(newEntityLabel);
            }
        }

        for (const auto& [type, metadata] : NexusEngine::MetadataRegistry::Instance().GetAll())
        {
            (void)type;
            if (!metadata.m_hasComponent || !metadata.m_hasComponent(entity))
            {
                continue;
            }

            const std::optional<NexusEngine::ComponentView> componentView = NexusEngine::CreateEntityComponentView(metadata, entity);
            if (!componentView)
            {
                continue;
            }

            NexusEngine::ForEachField(
                *componentView,
                [this, &metadata](const NexusEngine::FieldView& field)
                {
                    const QString objectName = MakePropertyControlObjectName(metadata.m_name, std::string(field.m_name));
                    const bool isReadOnly = NexusEngine::IsFieldReadOnly(field);
                    if (NexusEngine::GetFieldValueKind(field) == NexusEngine::FieldValueKind::Bool)
                    {
                        if (auto* checkBox = findChild<QCheckBox*>(objectName))
                        {
                            const bool isChecked = NexusEngine::ReadFieldBool(field).value_or(false);
                            QSignalBlocker blocker(checkBox);
                            checkBox->setChecked(isChecked);
                            checkBox->setEnabled(!isReadOnly);
                        }
                    }
                    else
                    {
                        if (auto* lineEdit = findChild<QLineEdit*>(objectName))
                        {
                            const QString newValue = QString::fromStdString(NexusEngine::ReadFieldText(field).value_or(std::string{}));
                            QSignalBlocker blocker(lineEdit);
                            if (lineEdit->text() != newValue)
                            {
                                lineEdit->setText(newValue);
                            }
                            lineEdit->setReadOnly(isReadOnly);
                        }
                    }
                });
        }
    }

    void PropertyWidget::RebuildContents()
    {
        ClearLayout(m_contentLayout);

        if (!m_editorWindow || !m_editorWindow->IsEngineInitialized())
        {
            m_contentLayout->addWidget(new QLabel(QStringLiteral("Initializing properties..."), this));
            return;
        }

        NexusEngine::Scene* activeScene = m_editorWindow->GetActiveScene();
        if (!activeScene)
        {
            m_contentLayout->addWidget(new QLabel(QStringLiteral("No active scene"), this));
            return;
        }

        if (m_selectedEntityId == 0)
        {
            m_contentLayout->addWidget(new QLabel(QStringLiteral("Select an entity to inspect"), this));
            return;
        }

        const flecs::entity entity = activeScene->m_world.entity(static_cast<flecs::entity_t>(m_selectedEntityId));
        if (!entity.is_valid() || !entity.is_alive())
        {
            m_selectedEntityId = 0;
            m_contentLayout->addWidget(new QLabel(QStringLiteral("Select an entity to inspect"), this));
            return;
        }

        if (entity.has<NexusEngine::EditorOnlyComponent>())
        {
            m_selectedEntityId = 0;
            m_contentLayout->addWidget(new QLabel(QStringLiteral("Select an entity to inspect"), this));
            return;
        }

        const char* entityName = entity.name();
        auto* entityNameLabel = new QLabel(
            entityName && entityName[0] != '\0'
                ? QString::fromUtf8(entityName)
                : QStringLiteral("Entity %1").arg(static_cast<qulonglong>(m_selectedEntityId)),
            this);
        entityNameLabel->setObjectName(QString::fromUtf8(EntityNameLabelObjectName));
        m_contentLayout->addWidget(entityNameLabel);

        bool addedAnyComponents = false;
        for (const auto& [type, metadata] : NexusEngine::MetadataRegistry::Instance().GetAll())
        {
            (void)type;
            if (!metadata.m_hasComponent || !metadata.m_hasComponent(entity))
            {
                continue;
            }

            const std::optional<NexusEngine::ComponentView> componentView = NexusEngine::CreateEntityComponentView(metadata, entity);
            if (!componentView)
            {
                continue;
            }

            auto* groupBox = new QGroupBox(QString::fromStdString(metadata.m_name), this);
            auto* formLayout = new QFormLayout(groupBox);

            NexusEngine::ForEachField(
                *componentView,
                [groupBox, formLayout, entity, &metadata](const NexusEngine::FieldView& field)
                {
                    const std::string fieldName(field.m_name);
                    const QString labelText = QStringLiteral("%1 (%2)")
                        .arg(QString::fromStdString(fieldName), GetInspectorFieldTypeLabel(field));
                    const bool isReadOnly = NexusEngine::IsFieldReadOnly(field);
                    if (NexusEngine::GetFieldValueKind(field) == NexusEngine::FieldValueKind::Bool)
                    {
                        auto* checkBox = new QCheckBox(groupBox);
                        checkBox->setObjectName(MakePropertyControlObjectName(metadata.m_name, fieldName));
                        checkBox->setChecked(NexusEngine::ReadFieldBool(field).value_or(false));
                        checkBox->setEnabled(!isReadOnly);
                        if (!isReadOnly)
                        {
                            const NexusEngine::ComponentMetadata* metadataPtr = &metadata;
                            connect(checkBox, &QCheckBox::toggled, groupBox, [entity, metadataPtr, fieldName](bool checked)
                                {
                                    (void)NexusEngine::WriteFieldBool(*metadataPtr, entity, fieldName, checked);
                                });
                        }
                        formLayout->addRow(labelText, checkBox);
                    }
                    else
                    {
                        auto* lineEdit = new QLineEdit(groupBox);
                        lineEdit->setObjectName(MakePropertyControlObjectName(metadata.m_name, fieldName));
                        lineEdit->setText(QString::fromStdString(NexusEngine::ReadFieldText(field).value_or(std::string{})));
                        lineEdit->setReadOnly(isReadOnly);
                        if (!isReadOnly)
                        {
                            const NexusEngine::ComponentMetadata* metadataPtr = &metadata;
                            connect(lineEdit, &QLineEdit::editingFinished, groupBox, [entity, lineEdit, metadataPtr, fieldName]()
                                {
                                    (void)NexusEngine::WriteFieldText(*metadataPtr, entity, fieldName, lineEdit->text().toStdString());
                                });
                        }
                        formLayout->addRow(labelText, lineEdit);
                    }
                });

            m_contentLayout->addWidget(groupBox);
            addedAnyComponents = true;
        }

        if (!addedAnyComponents)
        {
            m_contentLayout->addWidget(new QLabel(QStringLiteral("Entity has no inspectable components"), this));
        }
    }

    void PropertyWidget::RebuildAddComponentOptions()
    {
        m_addComponentComboBox->clear();

        if (!m_editorWindow || !m_editorWindow->IsEngineInitialized() || m_selectedEntityId == 0)
        {
            return;
        }

        NexusEngine::Scene* activeScene = m_editorWindow->GetActiveScene();
        if (!activeScene)
        {
            return;
        }

        const flecs::entity entity = activeScene->m_world.entity(static_cast<flecs::entity_t>(m_selectedEntityId));
        if (!entity.is_valid() || !entity.is_alive())
        {
            return;
        }

        if (entity.has<NexusEngine::EditorOnlyComponent>())
        {
            return;
        }

        for (const auto& [type, metadata] : NexusEngine::MetadataRegistry::Instance().GetAll())
        {
            (void)type;
            if (!metadata.m_addComponent || (metadata.m_hasComponent && metadata.m_hasComponent(entity)))
            {
                continue;
            }

            m_addComponentComboBox->addItem(QString::fromStdString(metadata.m_name), QString::fromStdString(metadata.m_name));
        }
    }
} // namespace NexusEditor
