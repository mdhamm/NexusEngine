#include "PropertyWidget.h"

#include "EditorSceneSerializer.h"
#include "EditorWindow.h"
#include "PropertyWidgetSerializer.h"

#include <assets/MaterialAsset.h>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#ifdef emit
#undef emit
#endif

#include <Scene.h>
#include <components/EditorOnlyComponent.h>
#include <reflection/EntityReflection.h>
#include <reflection/MetadataHelpers.h>

#include <filesystem/AssetReferenceRegistry.h>

#include <algorithm>
#include <cstdint>
#include <string>

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
        , m_propertyWidgetSerializer(std::make_unique<PropertyWidgetSerializer>(*this))
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
                if (!m_editorWindow || m_selectedEntityId == 0)
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

    bool PropertyWidget::TryAssignPickedAssetPath(const QString& assetPath)
    {
        if (!m_pendingAssetReferencePick || !m_editorWindow || m_selectedEntityId == 0)
        {
            return false;
        }

        if (!IsAcceptedAssetPath(assetPath, m_pendingAssetReferencePick->m_assetType))
        {
            return false;
        }

        NexusEngine::Scene* activeScene = m_editorWindow->GetActiveScene();
        if (!activeScene)
        {
            return false;
        }

        const flecs::entity entity = activeScene->m_world.entity(static_cast<flecs::entity_t>(m_selectedEntityId));
        if (!entity.is_valid() || !entity.is_alive())
        {
            return false;
        }

        const NexusEngine::ComponentMetadata* metadata = NexusEngine::MetadataRegistry::Instance().FindByName(m_pendingAssetReferencePick->m_componentName);
        if (!metadata)
        {
            return false;
        }

        if (!AssignAssetReferencePath(*metadata, entity, m_pendingAssetReferencePick->m_fieldName, assetPath))
        {
            return false;
        }

        ClearAssetReferencePick();
        SyncDisplayedValues();
        return true;
    }

    void PropertyWidget::NotifyEntityDeleted(std::uint64_t entityId)
    {
        if (m_selectedEntityId != entityId)
        {
            return;
        }

        m_selectedEntityId = 0;
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

        if (TryAssignPickedAssetPath(cleanAssetPath))
        {
            return;
        }

        m_selectedAssetPath = cleanAssetPath;
        m_selectedEntityId = 0;
        Refresh();
    }

    void PropertyWidget::Refresh()
    {
        const bool isMaterialAssetSelected = NexusEngine::IsMaterialAssetFilePath(m_selectedAssetPath.toStdString());
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

        if (NexusEngine::IsMaterialAssetFilePath(m_selectedAssetPath.toStdString()))
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
        if (!m_editorWindow)
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
                    const std::string fieldName = field.m_meta ? field.m_meta->m_name : std::string{};
                    signature += QStringLiteral("|property:%1:%2:%3:%4")
                        .arg(QString::fromStdString(fieldName))
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

    QStringList PropertyWidget::GetAssetPathsForType(NexusEngine::AssetType assetType) const
    {
        QStringList assetPaths;
        if (!m_editorWindow)
        {
            return assetPaths;
        }

        const QString projectRootPath = QDir::cleanPath(m_editorWindow->GetProjectRootPath());
        QDirIterator iterator(projectRootPath, QDir::Files, QDirIterator::Subdirectories);
        while (iterator.hasNext())
        {
            const QString absolutePath = QDir::cleanPath(iterator.next());
            if (IsAcceptedAssetPath(absolutePath, assetType))
            {
                assetPaths.push_back(NormalizeAssetPath(absolutePath));
            }
        }

        assetPaths.removeDuplicates();
        assetPaths.sort(Qt::CaseInsensitive);
        return assetPaths;
    }

    QString PropertyWidget::GetAssetReferenceDisplayPath(const std::string& assetReferenceGuid) const
    {
        if (!m_editorWindow || assetReferenceGuid.empty())
        {
            return {};
        }

        NexusEngine::Engine* engine = m_editorWindow->GetEngine();
        if (!engine)
        {
            return {};
        }

        NexusEngine::IO::AssetReferenceRegistry* assetReferenceRegistry = engine->GetAssetReferenceRegistry();
        if (!assetReferenceRegistry)
        {
            return {};
        }

        const std::filesystem::path resolvedPath = assetReferenceRegistry->ResolveAssetReferencePath(NexusEngine::IO::AssetReference{ assetReferenceGuid });
        return resolvedPath.empty()
            ? QString{}
            : QString::fromStdWString(resolvedPath.wstring());
    }

    bool PropertyWidget::AssignAssetReferencePath(
        const NexusEngine::ComponentMetadata& metadata,
        const flecs::entity& entity,
        const std::string& fieldName,
        const QString& assetPath) const
    {
        if (!m_editorWindow)
        {
            return false;
        }

        NexusEngine::Engine* engine = m_editorWindow->GetEngine();
        if (!engine)
        {
            return false;
        }

        NexusEngine::IO::AssetReferenceRegistry* assetReferenceRegistry = engine->GetAssetReferenceRegistry();
        if (!assetReferenceRegistry)
        {
            return false;
        }

        const QString normalizedPath = NormalizeAssetPath(assetPath);
        const QString absolutePath = QDir::cleanPath(QFileInfo(normalizedPath).isAbsolute()
            ? normalizedPath
            : QDir(m_editorWindow->GetProjectRootPath()).filePath(normalizedPath));
        const NexusEngine::IO::AssetReference assetReference = assetReferenceRegistry->GetOrCreateAssetReferenceByPath(std::filesystem::path(absolutePath.toStdWString()));
        if (assetReference.IsEmpty())
        {
            return false;
        }

        return NexusEngine::WriteFieldText(metadata, entity, fieldName, assetReference.GetGuid());
    }

    QString PropertyWidget::NormalizeAssetPath(const QString& assetPath) const
    {
        const QString trimmedPath = assetPath.trimmed();
        if (!m_editorWindow || trimmedPath.isEmpty())
        {
            return QDir::cleanPath(trimmedPath);
        }

        const QString projectRootPath = QDir::cleanPath(m_editorWindow->GetProjectRootPath());
        const QString cleanPath = QDir::cleanPath(trimmedPath);
        const QFileInfo fileInfo(cleanPath);
        if (fileInfo.isAbsolute())
        {
            return QDir(projectRootPath).relativeFilePath(cleanPath);
        }

        return cleanPath;
    }

    bool PropertyWidget::IsAcceptedAssetPath(const QString& assetPath, NexusEngine::AssetType assetType) const
    {
        const QString normalizedPath = NormalizeAssetPath(assetPath);
        if (normalizedPath.isEmpty())
        {
            return false;
        }

        switch (assetType)
        {
        case NexusEngine::AssetType::Material:
            return NexusEngine::IsMaterialAssetFilePath(normalizedPath.toStdString());
        case NexusEngine::AssetType::Scene:
            return IsSceneFilePath(normalizedPath);
        case NexusEngine::AssetType::Mesh:
        {
            const QString lowerPath = normalizedPath.toLower();
            return lowerPath.endsWith(QStringLiteral(".obj"))
                || lowerPath.endsWith(QStringLiteral(".fbx"))
                || lowerPath.endsWith(QStringLiteral(".gltf"))
                || lowerPath.endsWith(QStringLiteral(".glb"))
                || lowerPath.endsWith(QStringLiteral(".dae"))
                || lowerPath.endsWith(QStringLiteral(".ply"))
                || lowerPath.endsWith(QStringLiteral(".stl"))
                || lowerPath.endsWith(QStringLiteral(".3ds"));
        }
        default:
            return false;
        }
    }

    void PropertyWidget::BeginAssetReferencePick(
        const std::string& componentName,
        const std::string& fieldName,
        NexusEngine::AssetType assetType,
        const QString& controlObjectName)
    {
        m_pendingAssetReferencePick = PendingAssetReferencePick{ componentName, fieldName, assetType, controlObjectName };
        SyncDisplayedValues();
    }

    void PropertyWidget::RemoveComponent(const std::string& componentName)
    {
        if (!m_editorWindow || m_selectedEntityId == 0)
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

        const NexusEngine::ComponentMetadata* metadata = NexusEngine::MetadataRegistry::Instance().FindByName(componentName);
        if (!metadata || !metadata->m_removeComponent)
        {
            return;
        }

        metadata->m_removeComponent(entity);
        Refresh();
    }

    bool PropertyWidget::IsAssetReferencePickActive(const QString& controlObjectName) const
    {
        return m_pendingAssetReferencePick.has_value()
            && m_pendingAssetReferencePick->m_controlObjectName == controlObjectName;
    }

    void PropertyWidget::ClearAssetReferencePick()
    {
        if (!m_pendingAssetReferencePick)
        {
            return;
        }

        m_pendingAssetReferencePick.reset();
        SyncDisplayedValues();
    }

    void PropertyWidget::RebuildAssetContents()
    {
        ClearLayout(m_contentLayout);

        NexusEngine::MaterialAsset materialData;
        if (!LoadMaterialAssetFile(std::filesystem::path(m_selectedAssetPath.toStdWString()), materialData))
        {
            m_contentLayout->addWidget(new QLabel(QStringLiteral("Select an entity to inspect"), this));
            return;
        }

        auto* assetNameLabel = new QLabel(QString::fromStdString(materialData.m_name), this);
        m_contentLayout->addWidget(assetNameLabel);

        auto* groupBox = new QGroupBox(QStringLiteral("Material"), this);
        auto* formLayout = new QFormLayout(groupBox);

        auto saveMaterial = [this](const NexusEngine::MaterialAsset& updatedMaterialData)
        {
            (void)SaveMaterialAssetFile(std::filesystem::path(m_selectedAssetPath.toStdWString()), updatedMaterialData);
        };

        auto* vertexShaderLineEdit = new QLineEdit(QString::fromStdString(materialData.m_vertexShaderPath), groupBox);
        connect(vertexShaderLineEdit, &QLineEdit::editingFinished, groupBox, [saveMaterial, materialData, vertexShaderLineEdit]() mutable
            {
                materialData.m_vertexShaderPath = vertexShaderLineEdit->text().trimmed().toStdString();
                saveMaterial(materialData);
            });
        formLayout->addRow(QStringLiteral("Vertex Shader (string)"), vertexShaderLineEdit);

        auto* pixelShaderLineEdit = new QLineEdit(QString::fromStdString(materialData.m_pixelShaderPath), groupBox);
        connect(pixelShaderLineEdit, &QLineEdit::editingFinished, groupBox, [saveMaterial, materialData, pixelShaderLineEdit]() mutable
            {
                materialData.m_pixelShaderPath = pixelShaderLineEdit->text().trimmed().toStdString();
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
        cullModeComboBox->setCurrentText(QString::fromStdString(materialData.m_cullMode));
        connect(cullModeComboBox, &QComboBox::currentTextChanged, groupBox, [saveMaterial, materialData](const QString& text) mutable
            {
                materialData.m_cullMode = text.toStdString();
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
        if (!m_editorWindow)
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
                    const std::string fieldName = field.m_meta ? field.m_meta->m_name : std::string{};
                    const QString objectName = MakePropertyControlObjectName(metadata.m_name, fieldName);
                    if (QWidget* editor = findChild<QWidget*>(objectName))
                    {
                        m_propertyWidgetSerializer->SyncFieldEditor(field, editor);
                    }
                });
        }
    }

    void PropertyWidget::RebuildContents()
    {
        ClearLayout(m_contentLayout);

        if (!m_editorWindow)
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
            auto* removeComponentButton = new QPushButton(QStringLiteral("Remove Component"), groupBox);
            removeComponentButton->setEnabled(metadata.m_removeComponent != nullptr);
            connect(removeComponentButton, &QPushButton::clicked, groupBox, [this, componentName = metadata.m_name]()
                {
                    RemoveComponent(componentName);
                });
            formLayout->addRow(QString{}, removeComponentButton);

            NexusEngine::ForEachField(
                *componentView,
                [this, groupBox, formLayout, entity, &metadata](const NexusEngine::FieldView& field)
                {
                    if (m_propertyWidgetSerializer)
                    {
                        AddPropertyWidgetFieldRow(*m_propertyWidgetSerializer, formLayout, groupBox, entity, metadata, field);
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

        if (!m_editorWindow || m_selectedEntityId == 0)
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
