#include "ContentDrawerWidget.h"

#include "EditorMaterialSerializer.h"
#include "EditorSceneSerializer.h"

#include <filesystem/AssetReferenceRegistry.h>
#include <QDir>
#include <QDropEvent>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListView>
#include <QMenu>
#include <QPushButton>
#include <QSplitter>
#include <QTreeView>

namespace NexusEditor
{
    namespace
    {
        class AssetFolderModel : public QFileSystemModel
        {
        public:
            using QFileSystemModel::QFileSystemModel;

            QString RootPath;

            QVariant data(const QModelIndex& index, int role) const override
            {
                if (role == Qt::DisplayRole &&
                    QDir::cleanPath(filePath(index)).compare(QDir::cleanPath(RootPath), Qt::CaseInsensitive) == 0)
                {
                    return QStringLiteral("Project Root");
                }

                return QFileSystemModel::data(index, role);
            }
        };

        struct PendingAssetDrag
        {
            QStringList m_paths;
        };

        PendingAssetDrag& GetPendingAssetDrag()
        {
            static PendingAssetDrag s_pendingAssetDrag;
            return s_pendingAssetDrag;
        }

        class AssetFolderTreeView final : public QTreeView
        {
        public:
            using QTreeView::QTreeView;

            std::function<QString(const QModelIndex&)> m_getPath;
            std::function<void(const QString&, const QString&)> m_onAssetMoved;

        protected:
            void startDrag(Qt::DropActions supportedActions) override
            {
                PendingAssetDrag& pendingDrag = GetPendingAssetDrag();
                pendingDrag.m_paths.clear();

                const QModelIndexList selectedIndexes = selectionModel() ? selectionModel()->selectedRows() : QModelIndexList{};
                for (const QModelIndex& index : selectedIndexes)
                {
                    if (!index.isValid() || !m_getPath)
                    {
                        continue;
                    }

                    const QString path = QDir::cleanPath(m_getPath(index));
                    if (!path.isEmpty() && !pendingDrag.m_paths.contains(path))
                    {
                        pendingDrag.m_paths.push_back(path);
                    }
                }

                QTreeView::startDrag(supportedActions);
            }

            void dropEvent(QDropEvent* event) override
            {
                const QStringList draggedPaths = GetPendingAssetDrag().m_paths;
                const QString targetDirectory = m_getPath
                    ? QDir::cleanPath(m_getPath(indexAt(event->position().toPoint())))
                    : QString{};
                const bool requestedMove = event->proposedAction() == Qt::MoveAction || event->dropAction() == Qt::MoveAction;

                QTreeView::dropEvent(event);

                if (requestedMove && event->isAccepted() && !targetDirectory.isEmpty() && m_onAssetMoved)
                {
                    for (const QString& oldPath : draggedPaths)
                    {
                        const QString newPath = QDir(targetDirectory).filePath(QFileInfo(oldPath).fileName());
                        if (QFileInfo::exists(newPath) && QDir::cleanPath(oldPath).compare(QDir::cleanPath(newPath), Qt::CaseInsensitive) != 0)
                        {
                            m_onAssetMoved(QDir::cleanPath(oldPath), QDir::cleanPath(newPath));
                        }
                    }
                }

                GetPendingAssetDrag().m_paths.clear();
            }
        };

        class AssetContentListView final : public QListView
        {
        public:
            using QListView::QListView;

            std::function<QString(const QModelIndex&)> m_getPath;
            std::function<bool(const QModelIndex&)> m_isDirectory;
            std::function<void(const QString&, const QString&)> m_onAssetMoved;

        protected:
            void startDrag(Qt::DropActions supportedActions) override
            {
                PendingAssetDrag& pendingDrag = GetPendingAssetDrag();
                pendingDrag.m_paths.clear();

                const QModelIndexList selectedIndexes = selectionModel() ? selectionModel()->selectedIndexes() : QModelIndexList{};
                for (const QModelIndex& index : selectedIndexes)
                {
                    if (!index.isValid() || !m_getPath)
                    {
                        continue;
                    }

                    const QString path = QDir::cleanPath(m_getPath(index));
                    if (!path.isEmpty() && !pendingDrag.m_paths.contains(path))
                    {
                        pendingDrag.m_paths.push_back(path);
                    }
                }

                QListView::startDrag(supportedActions);
            }

            void dropEvent(QDropEvent* event) override
            {
                const QStringList draggedPaths = GetPendingAssetDrag().m_paths;
                const QModelIndex dropIndex = indexAt(event->position().toPoint());
                QString targetDirectory = m_getPath ? QDir::cleanPath(m_getPath(rootIndex())) : QString{};
                if (dropIndex.isValid() && m_getPath && m_isDirectory && m_isDirectory(dropIndex))
                {
                    targetDirectory = QDir::cleanPath(m_getPath(dropIndex));
                }

                const bool requestedMove = event->proposedAction() == Qt::MoveAction || event->dropAction() == Qt::MoveAction;

                QListView::dropEvent(event);

                if (requestedMove && event->isAccepted() && !targetDirectory.isEmpty() && m_onAssetMoved)
                {
                    for (const QString& oldPath : draggedPaths)
                    {
                        const QString newPath = QDir(targetDirectory).filePath(QFileInfo(oldPath).fileName());
                        if (QFileInfo::exists(newPath) && QDir::cleanPath(oldPath).compare(QDir::cleanPath(newPath), Qt::CaseInsensitive) != 0)
                        {
                            m_onAssetMoved(QDir::cleanPath(oldPath), QDir::cleanPath(newPath));
                        }
                    }
                }

                GetPendingAssetDrag().m_paths.clear();
            }
        };
    }

    ContentDrawerWidget::ContentDrawerWidget(const QString& contentRootPath, QWidget* parent)
        : QWidget(parent)
        , m_contentRootPath(QDir::cleanPath(contentRootPath))
    {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        auto* splitter = new QSplitter(Qt::Horizontal, this);
        layout->addWidget(splitter);

        auto* folderModel = new AssetFolderModel(this);
        folderModel->RootPath = m_contentRootPath;
        folderModel->setReadOnly(false);
        folderModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
        folderModel->setRootPath(m_contentRootPath);
        m_folderModel = folderModel;

        m_contentModel = new QFileSystemModel(this);
        m_contentModel->setReadOnly(false);
        m_contentModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
        m_contentModel->setRootPath(m_contentRootPath);

        auto* folderTreeView = new AssetFolderTreeView(splitter);
        m_folderTreeView = folderTreeView;
        m_folderTreeView->setModel(m_folderModel);
        m_folderTreeView->setRootIndex(
            m_folderModel->index(QFileInfo(m_contentRootPath).absolutePath())
        );
        QModelIndex projectIndex = m_folderModel->index(m_contentRootPath);
        m_folderTreeView->expand(projectIndex);
        m_folderTreeView->setHeaderHidden(true);
        m_folderTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
        m_folderTreeView->setDragEnabled(true);
        m_folderTreeView->setAcceptDrops(true);
        m_folderTreeView->setDropIndicatorShown(true);
        m_folderTreeView->setDragDropMode(QAbstractItemView::DragDrop);
        m_folderTreeView->setDefaultDropAction(Qt::MoveAction);
        for (int column = 1; column < m_folderModel->columnCount(); ++column)
        {
            m_folderTreeView->hideColumn(column);
        }
        folderTreeView->m_getPath = [this](const QModelIndex& index)
        {
            return index.isValid() ? m_folderModel->filePath(index) : m_currentFolderPath;
        };
        folderTreeView->m_onAssetMoved = [this](const QString& oldPath, const QString& newPath)
        {
            (void)NexusEngine::IO::NotifyAssetPathChanged(oldPath.toStdString(), newPath.toStdString());
            if (m_onAssetRenamed)
            {
                m_onAssetRenamed(oldPath, newPath);
            }
        };

        auto* contentPane = new QWidget(splitter);
        auto* contentPaneLayout = new QVBoxLayout(contentPane);
        contentPaneLayout->setContentsMargins(0, 0, 0, 0);

        m_breadcrumbWidget = new QWidget(contentPane);
        m_breadcrumbLayout = new QHBoxLayout(m_breadcrumbWidget);
        m_breadcrumbLayout->setContentsMargins(0, 0, 0, 0);
        m_breadcrumbLayout->setSpacing(4);
        contentPaneLayout->addWidget(m_breadcrumbWidget);

        m_projectRootButton = new QPushButton(QStringLiteral("Project Root"), m_breadcrumbWidget);
        m_projectRootButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        m_breadcrumbLayout->addWidget(m_projectRootButton);
        m_breadcrumbLayout->addStretch(1);

        auto* contentListView = new AssetContentListView(contentPane);
        m_contentListView = contentListView;
        m_contentListView->setModel(m_contentModel);
        m_contentListView->setViewMode(QListView::IconMode);
        m_contentListView->setResizeMode(QListView::Adjust);
        m_contentListView->setContextMenuPolicy(Qt::CustomContextMenu);
        m_contentListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_contentListView->setDragEnabled(true);
        m_contentListView->setAcceptDrops(true);
        m_contentListView->setDropIndicatorShown(true);
        m_contentListView->setDragDropMode(QAbstractItemView::DragDrop);
        m_contentListView->setDefaultDropAction(Qt::MoveAction);
        contentPaneLayout->addWidget(m_contentListView, 1);
        contentListView->m_getPath = [this](const QModelIndex& index)
        {
            return index.isValid() ? m_contentModel->filePath(index) : m_currentFolderPath;
        };
        contentListView->m_isDirectory = [this](const QModelIndex& index)
        {
            return index.isValid() && m_contentModel->isDir(index);
        };
        contentListView->m_onAssetMoved = [this](const QString& oldPath, const QString& newPath)
        {
            (void)NexusEngine::IO::NotifyAssetPathChanged(oldPath.toStdString(), newPath.toStdString());
            if (m_onAssetRenamed)
            {
                m_onAssetRenamed(oldPath, newPath);
            }
        };

        splitter->setStretchFactor(0, 0);
        splitter->setStretchFactor(1, 1);

        connect(m_folderTreeView, &QTreeView::clicked, this,
            [this](const QModelIndex& index)
            {
                SetCurrentFolder(m_folderModel->filePath(index));
            });

        connect(m_folderTreeView, &QWidget::customContextMenuRequested, this,
            [this](const QPoint& position)
            {
                ShowFolderContextMenu(position);
            });

        connect(m_projectRootButton, &QPushButton::clicked, this,
            [this]()
            {
                SetCurrentFolder(m_contentRootPath);
            });

        connect(m_contentListView, &QListView::doubleClicked, this,
            [this](const QModelIndex& index)
            {
                if (!index.isValid())
                {
                    return;
                }

                const QFileInfo fileInfo = m_contentModel->fileInfo(index);
                if (fileInfo.isDir())
                {
                    SetCurrentFolder(fileInfo.absoluteFilePath());
                    return;
                }

                if (IsSceneIndex(index) && m_onSceneOpened)
                {
                    m_onSceneOpened(fileInfo.absoluteFilePath());
                }
            });

        connect(m_contentListView, &QListView::clicked, this,
            [this](const QModelIndex& index)
            {
                if (!m_onAssetSelected)
                {
                    return;
                }

                if (!index.isValid() || m_contentModel->isDir(index))
                {
                    m_onAssetSelected(QString{});
                    return;
                }

                const QString filePath = m_contentModel->filePath(index);
                m_onAssetSelected(IsMaterialAssetFilePath(filePath) ? filePath : QString{});
            });

        connect(m_contentListView, &QWidget::customContextMenuRequested, this,
            [this](const QPoint& position)
            {
                ShowContentContextMenu(position);
            });

        connect(m_contentModel, &QFileSystemModel::fileRenamed, this,
            [this](const QString& path, const QString& oldName, const QString& newName)
            {
                const QString oldPath = QDir(path).filePath(oldName);
                const QString newPath = QDir(path).filePath(newName);
                (void)NexusEngine::IO::NotifyAssetPathChanged(oldPath.toStdString(), newPath.toStdString());

                if (m_onAssetRenamed)
                {
                    m_onAssetRenamed(QDir::cleanPath(oldPath), QDir::cleanPath(newPath));
                }
            });

        SetCurrentFolder(m_contentRootPath);
    }

    void ContentDrawerWidget::SetSceneOpenedCallback(std::function<void(const QString&)> callback)
    {
        m_onSceneOpened = std::move(callback);
    }

    void ContentDrawerWidget::SetAssetSelectedCallback(std::function<void(const QString&)> callback)
    {
        m_onAssetSelected = std::move(callback);
    }

    void ContentDrawerWidget::SetAssetRenamedCallback(std::function<void(const QString&, const QString&)> callback)
    {
        m_onAssetRenamed = std::move(callback);
    }

    void ContentDrawerWidget::SetCurrentFolder(const QString& folderPath)
    {
        const QString resolvedPath = folderPath.isEmpty() ? m_contentRootPath : QDir::cleanPath(folderPath);
        const QModelIndex folderIndex = m_folderModel->index(resolvedPath);
        const QModelIndex contentIndex = m_contentModel->index(resolvedPath);
        if (!folderIndex.isValid() || !contentIndex.isValid())
        {
            return;
        }

        m_folderTreeView->setCurrentIndex(folderIndex);
        m_folderTreeView->scrollTo(folderIndex);
        m_contentListView->setRootIndex(contentIndex);
        m_currentFolderPath = resolvedPath;
        RebuildBreadcrumbs(resolvedPath);

        if (m_onAssetSelected)
        {
            m_onAssetSelected(QString{});
        }
    }

    void ContentDrawerWidget::RebuildBreadcrumbs(const QString& folderPath)
    {
        while (m_breadcrumbLayout->count() > 0)
        {
            QLayoutItem* item = m_breadcrumbLayout->takeAt(0);
            if (QWidget* widget = item->widget())
            {
                if (widget != m_projectRootButton)
                {
                    widget->deleteLater();
                }
            }

            delete item;
        }

        m_breadcrumbLayout->addWidget(m_projectRootButton);

        const QString cleanRootPath = QDir::cleanPath(m_contentRootPath);
        const QString cleanFolderPath = QDir::cleanPath(folderPath);
        if (cleanFolderPath.compare(cleanRootPath, Qt::CaseInsensitive) != 0)
        {
            QDir rootDirectory(cleanRootPath);
            const QString relativePath = rootDirectory.relativeFilePath(cleanFolderPath);
            const QStringList pathSegments = relativePath.split(QDir::separator(), Qt::SkipEmptyParts);

            QString accumulatedPath = cleanRootPath;
            for (const QString& pathSegment : pathSegments)
            {
                auto* separatorLabel = new QLabel(QStringLiteral(">"), m_breadcrumbWidget);
                m_breadcrumbLayout->addWidget(separatorLabel);

                accumulatedPath = QDir(accumulatedPath).filePath(pathSegment);
                auto* crumbButton = new QPushButton(pathSegment, m_breadcrumbWidget);
                crumbButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
                connect(crumbButton, &QPushButton::clicked, this,
                    [this, accumulatedPath]()
                    {
                        SetCurrentFolder(accumulatedPath);
                    });
                m_breadcrumbLayout->addWidget(crumbButton);
            }
        }

        m_breadcrumbLayout->addStretch(1);
    }

    void ContentDrawerWidget::ShowFolderContextMenu(const QPoint& position)
    {
        const QModelIndex index = m_folderTreeView->indexAt(position);
        const QString folderPath = index.isValid() ? m_folderModel->filePath(index) : m_contentRootPath;

        QMenu menu(this);
        QMenu* createMenu = menu.addMenu(QStringLiteral("Create"));
        createMenu->addAction(QStringLiteral("Scene"), this, [this, folderPath]() { CreateSceneInDirectory(folderPath); });
        createMenu->addAction(QStringLiteral("Material"), this, [this, folderPath]() { CreateMaterialInDirectory(folderPath); });

        if (index.isValid())
        {
            menu.addAction(QStringLiteral("Rename"), this, [this, index]() { RenameIndex(index); });
        }

        menu.exec(m_folderTreeView->viewport()->mapToGlobal(position));
    }

    void ContentDrawerWidget::ShowContentContextMenu(const QPoint& position)
    {
        const QModelIndex index = m_contentListView->indexAt(position);
        QString targetDirectory = m_contentModel->filePath(m_contentListView->rootIndex());
        if (index.isValid() && m_contentModel->isDir(index))
        {
            targetDirectory = m_contentModel->filePath(index);
        }

        QMenu menu(this);
        QMenu* createMenu = menu.addMenu(QStringLiteral("New"));
        createMenu->addAction(QStringLiteral("Scene"), this, [this, targetDirectory]() { CreateSceneInDirectory(targetDirectory); });
        createMenu->addAction(QStringLiteral("Material"), this, [this, targetDirectory]() { CreateMaterialInDirectory(targetDirectory); });

        if (index.isValid())
        {
            menu.addAction(QStringLiteral("Rename"), this, [this, index]() { RenameIndex(index); });
            menu.addAction(QStringLiteral("Delete"), this, [this, index]() { DeleteIndex(index); });
        }

        menu.exec(m_contentListView->viewport()->mapToGlobal(position));
    }

    void ContentDrawerWidget::CreateSceneInDirectory(const QString& directoryPath)
    {
        const QString filePath = GetNextSceneFilePath(directoryPath);
        if (CreateEmptySceneFile(filePath, QFileInfo(filePath).completeBaseName()))
        {
            (void)NexusEngine::IO::CreateAssetReference(filePath.toStdString());
            SetCurrentFolder(directoryPath);
        }
    }

    void ContentDrawerWidget::CreateMaterialInDirectory(const QString& directoryPath)
    {
        const QString filePath = GetNextMaterialFilePath(directoryPath);
        if (CreateEmptyMaterialFile(filePath, QFileInfo(filePath).completeBaseName()))
        {
            (void)NexusEngine::IO::CreateAssetReference(filePath.toStdString());
            SetCurrentFolder(directoryPath);
            if (m_onAssetSelected)
            {
                m_onAssetSelected(filePath);
            }
        }
    }

    QString ContentDrawerWidget::GetNextSceneFilePath(const QString& directoryPath) const
    {
        const QString cleanDirectoryPath = QDir::cleanPath(directoryPath);
        const QString baseName = QStringLiteral("NewScene");
        QString candidateFilePath = QDir(cleanDirectoryPath).filePath(baseName + QStringLiteral(".nscene"));
        if (!QFileInfo::exists(candidateFilePath))
        {
            return candidateFilePath;
        }

        int suffix = 1;
        do
        {
            candidateFilePath = QDir(cleanDirectoryPath).filePath(
                QStringLiteral("%1_%2.nscene").arg(baseName).arg(suffix++));
        } while (QFileInfo::exists(candidateFilePath));

        return candidateFilePath;
    }

    QString ContentDrawerWidget::GetNextMaterialFilePath(const QString& directoryPath) const
    {
        const QString cleanDirectoryPath = QDir::cleanPath(directoryPath);
        const QString baseName = QStringLiteral("NewMaterial");
        QString candidateFilePath = QDir(cleanDirectoryPath).filePath(baseName + QStringLiteral(".nmat"));
        if (!QFileInfo::exists(candidateFilePath))
        {
            return candidateFilePath;
        }

        int suffix = 1;
        do
        {
            candidateFilePath = QDir(cleanDirectoryPath).filePath(
                QStringLiteral("%1_%2.nmat").arg(baseName).arg(suffix++));
        } while (QFileInfo::exists(candidateFilePath));

        return candidateFilePath;
    }

    void ContentDrawerWidget::RenameIndex(const QModelIndex& index)
    {
        if (!index.isValid())
        {
            return;
        }

        if (index.model() == m_folderModel)
        {
            m_folderTreeView->edit(index);
            return;
        }

        m_contentListView->edit(index);
    }

    void ContentDrawerWidget::DeleteIndex(const QModelIndex& index)
    {
        if (!index.isValid())
        {
            return;
        }

        if (index.model() == m_folderModel)
        {
            m_folderModel->remove(index);
            return;
        }

        m_contentModel->remove(index);
    }

    bool ContentDrawerWidget::IsSceneIndex(const QModelIndex& index) const
    {
        return index.isValid() && !m_contentModel->isDir(index) && IsSceneFilePath(m_contentModel->filePath(index));
    }
} // namespace NexusEditor
