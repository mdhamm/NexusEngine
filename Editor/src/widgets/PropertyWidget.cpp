#include "PropertyWidget.h"

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

#include <ComponentReflection.h>
#include <Scene.h>

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
    }

    PropertyWidget::PropertyWidget(EditorWindow& editorWindow, QWidget* parent)
        : QWidget(parent)
        , m_editorWindow(&editorWindow)
    {
        auto* rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(0, 0, 0, 0);

        auto* addComponentLayout = new QHBoxLayout();
        m_addComponentComboBox = new QComboBox(this);
        auto* addComponentButton = new QPushButton(QStringLiteral("Add Component"), this);
        addComponentLayout->addWidget(m_addComponentComboBox, 1);
        addComponentLayout->addWidget(addComponentButton);
        rootLayout->addLayout(addComponentLayout);

        m_contentLayout = new QVBoxLayout();
        rootLayout->addLayout(m_contentLayout);
        rootLayout->addStretch(1);

        connect(addComponentButton, &QPushButton::clicked, this, [this]()
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

                for (const auto& descriptor : NexusEngine::ComponentReflectionRegistry::Instance().GetDescriptors())
                {
                    if (descriptor.m_name == componentName && descriptor.m_addComponent)
                    {
                        descriptor.m_addComponent(entity);
                        break;
                    }
                }

                Refresh();
            });

        Refresh();
    }

    void PropertyWidget::SetSelectedEntityId(std::uint64_t entityId)
    {
        if (m_selectedEntityId == entityId)
        {
            return;
        }

        m_selectedEntityId = entityId;
        Refresh();
    }

    void PropertyWidget::Refresh()
    {
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

        for (const auto& descriptor : NexusEngine::ComponentReflectionRegistry::Instance().GetDescriptors())
        {
            if (!descriptor.m_hasComponent || !descriptor.m_hasComponent(entity))
            {
                continue;
            }

            signature += QStringLiteral("|component:%1").arg(QString::fromStdString(descriptor.m_name));

            const std::vector<NexusEngine::ComponentPropertyDescriptor> properties =
                descriptor.m_getProperties ? descriptor.m_getProperties(entity) : std::vector<NexusEngine::ComponentPropertyDescriptor>{};

            for (const auto& property : properties)
            {
                signature += QStringLiteral("|property:%1:%2:%3:%4")
                    .arg(QString::fromStdString(property.m_name))
                    .arg(QString::fromStdString(property.m_typeName))
                    .arg(static_cast<int>(property.m_valueType))
                    .arg(property.m_isReadOnly ? 1 : 0);
            }
        }

        signature += QStringLiteral("|addable:");
        for (const auto& descriptor : NexusEngine::ComponentReflectionRegistry::Instance().GetDescriptors())
        {
            if (!descriptor.m_addComponent || (descriptor.m_hasComponent && descriptor.m_hasComponent(entity)))
            {
                continue;
            }

            signature += QString::fromStdString(descriptor.m_name);
            signature += QLatin1Char(';');
        }

        return signature;
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

        for (const auto& descriptor : NexusEngine::ComponentReflectionRegistry::Instance().GetDescriptors())
        {
            if (!descriptor.m_hasComponent || !descriptor.m_hasComponent(entity))
            {
                continue;
            }

            const std::vector<NexusEngine::ComponentPropertyDescriptor> properties =
                descriptor.m_getProperties ? descriptor.m_getProperties(entity) : std::vector<NexusEngine::ComponentPropertyDescriptor>{};

            for (const auto& property : properties)
            {
                const QString objectName = MakePropertyControlObjectName(descriptor.m_name, property.m_name);
                if (property.m_valueType == NexusEngine::ComponentPropertyValueType::Bool)
                {
                    if (auto* checkBox = findChild<QCheckBox*>(objectName))
                    {
                        const bool isChecked = property.m_getValue && property.m_getValue(entity) == "true";
                        const bool isEnabled = !property.m_isReadOnly && static_cast<bool>(property.m_setValue);
                        QSignalBlocker blocker(checkBox);
                        checkBox->setChecked(isChecked);
                        checkBox->setEnabled(isEnabled);
                    }
                }
                else
                {
                    if (auto* lineEdit = findChild<QLineEdit*>(objectName))
                    {
                        const QString newValue = property.m_getValue ? QString::fromStdString(property.m_getValue(entity)) : QString{};
                        const bool isReadOnly = property.m_isReadOnly || !property.m_setValue;
                        QSignalBlocker blocker(lineEdit);
                        if (lineEdit->text() != newValue)
                        {
                            lineEdit->setText(newValue);
                        }
                        lineEdit->setReadOnly(isReadOnly);
                    }
                }
            }
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

        const char* entityName = entity.name();
        auto* entityNameLabel = new QLabel(
            entityName && entityName[0] != '\0'
                ? QString::fromUtf8(entityName)
                : QStringLiteral("Entity %1").arg(static_cast<qulonglong>(m_selectedEntityId)),
            this);
        entityNameLabel->setObjectName(QString::fromUtf8(EntityNameLabelObjectName));
        m_contentLayout->addWidget(entityNameLabel);

        bool addedAnyComponents = false;
        for (const auto& descriptor : NexusEngine::ComponentReflectionRegistry::Instance().GetDescriptors())
        {
            if (!descriptor.m_hasComponent || !descriptor.m_hasComponent(entity))
            {
                continue;
            }

            auto* groupBox = new QGroupBox(QString::fromStdString(descriptor.m_name), this);
            auto* formLayout = new QFormLayout(groupBox);

            const std::vector<NexusEngine::ComponentPropertyDescriptor> properties =
                descriptor.m_getProperties ? descriptor.m_getProperties(entity) : std::vector<NexusEngine::ComponentPropertyDescriptor>{};

            for (const auto& property : properties)
            {
                const QString labelText = QString::fromStdString(property.m_name + " (" + property.m_typeName + ")");
                if (property.m_valueType == NexusEngine::ComponentPropertyValueType::Bool)
                {
                    auto* checkBox = new QCheckBox(groupBox);
                    checkBox->setObjectName(MakePropertyControlObjectName(descriptor.m_name, property.m_name));
                    checkBox->setChecked(property.m_getValue && property.m_getValue(entity) == "true");
                    checkBox->setEnabled(!property.m_isReadOnly && static_cast<bool>(property.m_setValue));
                    if (!property.m_isReadOnly && property.m_setValue)
                    {
                        const auto setter = property.m_setValue;
                        connect(checkBox, &QCheckBox::toggled, groupBox, [entity, setter](bool checked)
                            {
                                setter(entity, checked ? std::string("true") : std::string("false"));
                            });
                    }
                    formLayout->addRow(labelText, checkBox);
                }
                else
                {
                    auto* lineEdit = new QLineEdit(groupBox);
                    lineEdit->setObjectName(MakePropertyControlObjectName(descriptor.m_name, property.m_name));
                    lineEdit->setText(property.m_getValue ? QString::fromStdString(property.m_getValue(entity)) : QString{});
                    lineEdit->setReadOnly(property.m_isReadOnly || !property.m_setValue);
                    if (!property.m_isReadOnly && property.m_setValue)
                    {
                        const auto setter = property.m_setValue;
                        connect(lineEdit, &QLineEdit::editingFinished, groupBox, [entity, lineEdit, setter]()
                            {
                                try
                                {
                                    setter(entity, lineEdit->text().toStdString());
                                }
                                catch (...)
                                {
                                }
                            });
                    }
                    formLayout->addRow(labelText, lineEdit);
                }
            }

            m_contentLayout->addWidget(groupBox);
            addedAnyComponents = true;
        }

        if (!addedAnyComponents)
        {
            m_contentLayout->addWidget(new QLabel(QStringLiteral("Entity has no reflected components"), this));
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

        for (const auto& descriptor : NexusEngine::ComponentReflectionRegistry::Instance().GetDescriptors())
        {
            if (!descriptor.m_addComponent || (descriptor.m_hasComponent && descriptor.m_hasComponent(entity)))
            {
                continue;
            }

            m_addComponentComboBox->addItem(QString::fromStdString(descriptor.m_name), QString::fromStdString(descriptor.m_name));
        }
    }
} // namespace NexusEditor
