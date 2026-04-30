#include "SceneGraphWidget.h"

#include "SceneViewWidget.h"

#include <Scene.h>

#include <QHeaderView>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <cstdint>
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

        m_treeWidget = new QTreeWidget(this);
        m_treeWidget->setColumnCount(1);
        m_treeWidget->setHeaderLabel(QStringLiteral("Scene"));
        m_treeWidget->header()->setStretchLastSection(true);
        layout->addWidget(m_treeWidget);

        auto* refreshTimer = new QTimer(this);
        refreshTimer->setInterval(500);
        connect(refreshTimer, &QTimer::timeout, this, [this]() { Refresh(); });
        refreshTimer->start();

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

        std::vector<flecs::entity> entities;
        activeScene->m_world.each(
            [&](flecs::entity entity)
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

        m_treeWidget->expandToDepth(1);
    }
} // namespace NexusEditor
