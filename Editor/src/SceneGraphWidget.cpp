#include "SceneGraphWidget.h"

#include "SceneViewWidget.h"

#include <Scene.h>
#include <components/TransformComponent.h>

#include <QHeaderView>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace NexusEditor
{
    namespace
    {
        QString GetEntityLabel(const flecs::entity& entity)
        {
            if (const char* name = entity.name(); name && name[0] != '\0')
            {
                return QString::fromUtf8(name);
            }

            return QStringLiteral("Entity %1").arg(static_cast<qulonglong>(entity.id()));
        }
    }

    SceneGraphWidget::SceneGraphWidget(SceneViewWidget* sceneView, QWidget* parent)
        : QWidget(parent)
        , m_sceneView(sceneView)
    {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        auto* addEntityButton = new QPushButton(QStringLiteral("Add Entity"), this);
        layout->addWidget(addEntityButton);

        m_treeWidget = new QTreeWidget(this);
        m_treeWidget->setColumnCount(1);
        m_treeWidget->setHeaderLabel(QStringLiteral("Scene"));
        m_treeWidget->header()->setStretchLastSection(true);
        layout->addWidget(m_treeWidget);

        connect(addEntityButton, &QPushButton::clicked, this, [this]()
            {
                static std::uint64_t s_entityCounter = 1;

                if (!m_sceneView || !m_sceneView->IsInitialized())
                {
                    return;
                }

                NexusEngine::Scene* activeScene = m_sceneView->GetActiveScene();
                if (!activeScene)
                {
                    return;
                }

                const std::string entityName = "Entity_" + std::to_string(static_cast<unsigned long long>(s_entityCounter++));
                const flecs::entity entity = activeScene->CreateEntity(entityName.c_str())
                    .set(NexusEngine::TransformComponent{});
                m_selectedEntityId = static_cast<std::uint64_t>(entity.id());
                Refresh();
                if (m_onSelectionChanged)
                {
                    m_onSelectionChanged(m_selectedEntityId);
                }
            });

        connect(m_treeWidget, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem* current, QTreeWidgetItem*)
            {
                m_selectedEntityId = current
                    ? static_cast<std::uint64_t>(current->data(0, Qt::UserRole).toULongLong())
                    : 0;

                if (m_onSelectionChanged)
                {
                    m_onSelectionChanged(m_selectedEntityId);
                }
            });

        Refresh();
    }

    void SceneGraphWidget::Refresh()
    {
        m_treeWidget->clear();

        if (!m_sceneView || !m_sceneView->IsInitialized())
        {
            m_treeWidget->addTopLevelItem(new QTreeWidgetItem(QStringList{ QStringLiteral("Initializing scene...") }));
            return;
        }

        NexusEngine::Engine* engine = m_sceneView->GetEngine();
        if (!engine)
        {
            m_treeWidget->addTopLevelItem(new QTreeWidgetItem(QStringList{ QStringLiteral("Engine unavailable") }));
            return;
        }

        NexusEngine::Scene* activeScene = engine->ActiveScene();
        if (!activeScene)
        {
            m_treeWidget->addTopLevelItem(new QTreeWidgetItem(QStringList{ QStringLiteral("No active scene") }));
            return;
        }

        m_treeWidget->setHeaderLabel(QStringLiteral("Scene - %1").arg(activeScene->m_name.c_str()));

        std::vector<flecs::entity> entities;
        activeScene->m_world.each<NexusEngine::TransformComponent>(
            [&](flecs::entity entity, NexusEngine::TransformComponent&)
            {
                if (entity.is_valid() && entity.is_alive())
                {
                    entities.push_back(entity);
                }
            });

        std::sort(
            entities.begin(),
            entities.end(),
            [](const flecs::entity& left, const flecs::entity& right)
            {
                return left.id() < right.id();
            });

        std::unordered_map<std::uint64_t, QTreeWidgetItem*> itemsById;
        itemsById.reserve(entities.size());

        for (const flecs::entity& entity : entities)
        {
            auto* item = new QTreeWidgetItem(QStringList{ GetEntityLabel(entity) });
            item->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<qulonglong>(entity.id())));
            itemsById.emplace(static_cast<std::uint64_t>(entity.id()), item);
        }

        for (const flecs::entity& entity : entities)
        {
            auto itemIt = itemsById.find(static_cast<std::uint64_t>(entity.id()));
            if (itemIt == itemsById.end())
            {
                continue;
            }

            QTreeWidgetItem* item = itemIt->second;
            const flecs::entity parent = entity.parent();
            if (parent.is_valid())
            {
                auto parentIt = itemsById.find(static_cast<std::uint64_t>(parent.id()));
                if (parentIt != itemsById.end())
                {
                    parentIt->second->addChild(item);
                    continue;
                }
            }

            m_treeWidget->addTopLevelItem(item);
        }

        if (m_selectedEntityId != 0)
        {
            auto selectedIt = itemsById.find(m_selectedEntityId);
            if (selectedIt != itemsById.end())
            {
                m_treeWidget->setCurrentItem(selectedIt->second);
                m_treeWidget->scrollToItem(selectedIt->second);
            }
        }

        m_treeWidget->expandToDepth(1);
    }

    void SceneGraphWidget::SetSelectionChangedCallback(std::function<void(std::uint64_t)> callback)
    {
        m_onSelectionChanged = std::move(callback);
    }
} // namespace NexusEditor
