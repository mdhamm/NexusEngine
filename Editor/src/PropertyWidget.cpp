#include "PropertyWidget.h"

#include "SceneViewWidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
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
    }

    PropertyWidget::PropertyWidget(SceneViewWidget* sceneView, QWidget* parent)
        : QWidget(parent)
        , m_sceneView(sceneView)
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
                if (!m_sceneView || !m_sceneView->IsInitialized() || m_selectedEntityId == 0)
                {
                    return;
                }

                NexusEngine::Scene* activeScene = m_sceneView->GetActiveScene();
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
    }

    void PropertyWidget::RebuildContents()
    {
        ClearLayout(m_contentLayout);

        if (!m_sceneView || !m_sceneView->IsInitialized())
        {
            m_contentLayout->addWidget(new QLabel(QStringLiteral("Initializing properties..."), this));
            return;
        }

        NexusEngine::Scene* activeScene = m_sceneView->GetActiveScene();
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
        m_contentLayout->addWidget(new QLabel(
            entityName && entityName[0] != '\0'
                ? QString::fromUtf8(entityName)
                : QStringLiteral("Entity %1").arg(static_cast<qulonglong>(m_selectedEntityId)),
            this));

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

        if (!m_sceneView || !m_sceneView->IsInitialized() || m_selectedEntityId == 0)
        {
            return;
        }

        NexusEngine::Scene* activeScene = m_sceneView->GetActiveScene();
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
