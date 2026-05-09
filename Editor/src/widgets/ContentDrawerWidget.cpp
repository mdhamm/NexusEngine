#include "ContentDrawerWidget.h"

#include "EditorSceneSerializer.h"

#include <assets/MaterialAsset.h>
#include <filesystem/AssetReferenceRegistry.h>
#include <QDir>
#include <QDropEvent>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QListView>
#include <QMenu>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStringList>
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

        class AssetFilterProxyModel final : public QSortFilterProxyModel
        {
        public:
            using QSortFilterProxyModel::QSortFilterProxyModel;

        protected:
            bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override
            {
                const auto* fileSystemModel = qobject_cast<const QFileSystemModel*>(sourceModel());
                if (!fileSystemModel)
                {
                    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
                }

                const QModelIndex sourceIndex = fileSystemModel->index(sourceRow, 0, sourceParent);
                if (!sourceIndex.isValid())
                {
                    return false;
                }

                const QString fileName = fileSystemModel->fileName(sourceIndex);
                if (fileName.compare(QStringLiteral(".nexus"), Qt::CaseInsensitive) == 0 ||
                    fileName.endsWith(QStringLiteral(".nmeta"), Qt::CaseInsensitive))
                {
                    return false;
                }

                return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
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
            std::function<void()> m_onDeleteSelectionRequested;

        protected:
            void keyPressEvent(QKeyEvent* event) override
            {
                if (event->key() == Qt::Key_Delete && m_onDeleteSelectionRequested)
                {
                    m_onDeleteSelectionRequested();
                    event->accept();
                    return;
                }

                QTreeView::keyPressEvent(event);
            }

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
            std::function<void()> m_onDeleteSelectionRequested;

        protected:
            void keyPressEvent(QKeyEvent* event) override
            {
                if (event->key() == Qt::Key_Delete && m_onDeleteSelectionRequested)
                {
                    m_onDeleteSelectionRequested();
                    event->accept();
                    return;
                }

                QListView::keyPressEvent(event);
            }

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

        m_folderProxyModel = new AssetFilterProxyModel(this);
        m_folderProxyModel->setSourceModel(m_folderModel);
        m_contentProxyModel = new AssetFilterProxyModel(this);
        m_contentProxyModel->setSourceModel(m_contentModel);

        auto* folderTreeView = new AssetFolderTreeView(splitter);
        m_folderTreeView = folderTreeView;
        m_folderTreeView->setModel(m_folderProxyModel);
        m_folderTreeView->setRootIndex(
            m_folderProxyModel->mapFromSource(m_folderModel->index(QFileInfo(m_contentRootPath).absolutePath()))
        );
        QModelIndex projectIndex = m_folderModel->index(m_contentRootPath);
        m_folderTreeView->expand(m_folderProxyModel->mapFromSource(projectIndex));
        m_folderTreeView->setHeaderHidden(true);
        m_folderTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
        m_folderTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_folderTreeView->setDragEnabled(true);
        m_folderTreeView->setAcceptDrops(true);
        m_folderTreeView->setDropIndicatorShown(true);
        m_folderTreeView->setDragDropMode(QAbstractItemView::DragDrop);
        m_folderTreeView->setDefaultDropAction(Qt::MoveAction);
        for (int column = 1; column < m_folderProxyModel->columnCount(); ++column)
        {
            m_folderTreeView->hideColumn(column);
        }
        folderTreeView->m_getPath = [this](const QModelIndex& index)
        {
            const QModelIndex sourceIndex = ToSourceIndex(index);
            return sourceIndex.isValid() ? m_folderModel->filePath(sourceIndex) : m_currentFolderPath;
        };
        folderTreeView->m_onAssetMoved = [this](const QString& oldPath, const QString& newPath)
        {
            if (m_onAssetRenamed)
            {
                m_onAssetRenamed(oldPath, newPath);
            }
        };
        folderTreeView->m_onDeleteSelectionRequested = [this]() { DeleteSelectedFolderIndexes(); };

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
        m_contentListView->setModel(m_contentProxyModel);
        m_contentListView->setViewMode(QListView::IconMode);
        m_contentListView->setResizeMode(QListView::Adjust);
        m_contentListView->setContextMenuPolicy(Qt::CustomContextMenu);
        m_contentListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_contentListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_contentListView->setSelectionRectVisible(true);
        m_contentListView->setDragEnabled(true);
        m_contentListView->setAcceptDrops(true);
        m_contentListView->setDropIndicatorShown(true);
        m_contentListView->setDragDropMode(QAbstractItemView::DragDrop);
        m_contentListView->setDefaultDropAction(Qt::MoveAction);
        contentPaneLayout->addWidget(m_contentListView, 1);
        contentListView->m_getPath = [this](const QModelIndex& index)
        {
            const QModelIndex sourceIndex = ToSourceIndex(index);
            return sourceIndex.isValid() ? m_contentModel->filePath(sourceIndex) : m_currentFolderPath;
        };
        contentListView->m_isDirectory = [this](const QModelIndex& index)
        {
            const QModelIndex sourceIndex = ToSourceIndex(index);
            return sourceIndex.isValid() && m_contentModel->isDir(sourceIndex);
        };
        contentListView->m_onAssetMoved = [this](const QString& oldPath, const QString& newPath)
        {
            if (m_onAssetRenamed)
            {
                m_onAssetRenamed(oldPath, newPath);
            }
        };
        contentListView->m_onDeleteSelectionRequested = [this]() { DeleteSelectedContentIndexes(); };

        splitter->setStretchFactor(0, 0);
        splitter->setStretchFactor(1, 1);

        connect(m_folderTreeView, &QTreeView::clicked, this,
            [this](const QModelIndex& index)
            {
                const QModelIndex sourceIndex = ToSourceIndex(index);
                SetCurrentFolder(m_folderModel->filePath(sourceIndex));
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

                const QModelIndex sourceIndex = ToSourceIndex(index);
                const QFileInfo fileInfo = m_contentModel->fileInfo(sourceIndex);
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

                const QModelIndex sourceIndex = ToSourceIndex(index);
                if (!sourceIndex.isValid() || m_contentModel->isDir(sourceIndex))
                {
                    m_onAssetSelected(QString{});
                    return;
                }

                const QString filePath = m_contentModel->filePath(sourceIndex);
                m_onAssetSelected(NexusEngine::IsMaterialAssetFilePath(filePath.toStdString()) ? filePath : QString{});
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

    void ContentDrawerWidget::SetAssetDeletedCallback(std::function<void(const QString&)> callback)
    {
        m_onAssetDeleted = std::move(callback);
    }

    void ContentDrawerWidget::SetCurrentFolder(const QString& folderPath)
    {
        const QString resolvedPath = folderPath.isEmpty() ? m_contentRootPath : QDir::cleanPath(folderPath);
        const QModelIndex folderSourceIndex = m_folderModel->index(resolvedPath);
        const QModelIndex contentSourceIndex = m_contentModel->index(resolvedPath);
        const QModelIndex folderIndex = m_folderProxyModel->mapFromSource(folderSourceIndex);
        const QModelIndex contentIndex = m_contentProxyModel->mapFromSource(contentSourceIndex);
        if (!folderSourceIndex.isValid() || !contentSourceIndex.isValid() || !folderIndex.isValid() || !contentIndex.isValid())
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
        const QModelIndex sourceIndex = ToSourceIndex(index);
        const QString folderPath = sourceIndex.isValid() ? m_folderModel->filePath(sourceIndex) : m_contentRootPath;

        QMenu menu(this);
        QMenu* createMenu = menu.addMenu(QStringLiteral("Create"));
        createMenu->addAction(QStringLiteral("Folder"), this, [this, folderPath]() { CreateFolderInDirectory(folderPath); });
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
        QString targetDirectory = m_contentModel->filePath(ToSourceIndex(m_contentListView->rootIndex()));
        const QModelIndex sourceIndex = ToSourceIndex(index);
        if (sourceIndex.isValid() && m_contentModel->isDir(sourceIndex))
        {
            targetDirectory = m_contentModel->filePath(sourceIndex);
        }

        QMenu menu(this);
        QMenu* createMenu = menu.addMenu(QStringLiteral("New"));
        createMenu->addAction(QStringLiteral("Folder"), this, [this, targetDirectory]() { CreateFolderInDirectory(targetDirectory); });
        createMenu->addAction(QStringLiteral("Scene"), this, [this, targetDirectory]() { CreateSceneInDirectory(targetDirectory); });
        createMenu->addAction(QStringLiteral("Material"), this, [this, targetDirectory]() { CreateMaterialInDirectory(targetDirectory); });

        if (index.isValid())
        {
            menu.addAction(QStringLiteral("Rename"), this, [this, sourceIndex]() { RenameIndex(sourceIndex); });
            menu.addAction(QStringLiteral("Delete"), this, [this, sourceIndex]() { DeleteIndex(sourceIndex); });
        }

        menu.exec(m_contentListView->viewport()->mapToGlobal(position));
    }

    void ContentDrawerWidget::CreateFolderInDirectory(const QString& directoryPath)
    {
        const QString folderPath = GetNextFolderPath(directoryPath);
        const QFileInfo folderInfo(folderPath);
        const QModelIndex parentIndex = m_folderModel->index(folderInfo.absolutePath());
        if (!parentIndex.isValid())
        {
            return;
        }

        const QModelIndex createdIndex = m_folderModel->mkdir(parentIndex, folderInfo.fileName());
        if (!createdIndex.isValid())
        {
            return;
        }

        SetCurrentFolder(directoryPath);
        m_folderTreeView->expand(parentIndex);
        m_folderTreeView->setCurrentIndex(createdIndex);
        m_folderTreeView->scrollTo(createdIndex);
        m_folderTreeView->edit(createdIndex);
    }

    void ContentDrawerWidget::CreateSceneInDirectory(const QString& directoryPath)
    {
        const QString filePath = GetNextSceneFilePath(directoryPath);
        if (CreateEmptySceneFile(filePath, QFileInfo(filePath).completeBaseName()))
        {
            SetCurrentFolder(directoryPath);
        }
    }

    void ContentDrawerWidget::CreateMaterialInDirectory(const QString& directoryPath)
    {
        const QString filePath = GetNextMaterialFilePath(directoryPath);
        if (NexusEngine::CreateEmptyMaterialAssetFile(std::filesystem::path(filePath.toStdWString()), QFileInfo(filePath).completeBaseName().toStdString()))
        {
            SetCurrentFolder(directoryPath);
            if (m_onAssetSelected)
            {
                m_onAssetSelected(filePath);
            }
        }
    }

    QString ContentDrawerWidget::GetNextFolderPath(const QString& directoryPath) const
    {
        const QString cleanDirectoryPath = QDir::cleanPath(directoryPath);
        const QString baseName = QStringLiteral("NewFolder");
        QString candidateFolderPath = QDir(cleanDirectoryPath).filePath(baseName);
        if (!QFileInfo::exists(candidateFolderPath))
        {
            return candidateFolderPath;
        }

        int suffix = 1;
        do
        {
            candidateFolderPath = QDir(cleanDirectoryPath).filePath(
                QStringLiteral("%1_%2").arg(baseName).arg(suffix++));
        } while (QFileInfo::exists(candidateFolderPath));

        return candidateFolderPath;
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

        const QModelIndex sourceIndex = ToSourceIndex(index);
        if (!sourceIndex.isValid())
        {
            return;
        }

        if (index.model() == m_folderModel)
        {
            m_folderTreeView->edit(m_folderProxyModel->mapFromSource(sourceIndex));
            return;
        }

        m_contentListView->edit(m_contentProxyModel->mapFromSource(sourceIndex));
    }

    void ContentDrawerWidget::DeleteIndex(const QModelIndex& index)
    {
        if (!index.isValid())
        {
            return;
        }

        const QModelIndex sourceIndex = ToSourceIndex(index);
        const QFileSystemModel* model = qobject_cast<const QFileSystemModel*>(sourceIndex.model());
        const QString path = model ? QDir::cleanPath(model->filePath(sourceIndex)) : QString{};
        if (!path.isEmpty() && m_onAssetDeleted)
        {
            m_onAssetDeleted(path);
        }

        if (sourceIndex.model() == m_folderModel)
        {
            m_folderModel->remove(sourceIndex);
            return;
        }

        m_contentModel->remove(sourceIndex);
    }

    void ContentDrawerWidget::DeleteSelectedFolderIndexes()
    {
        if (!m_folderTreeView || !m_folderTreeView->selectionModel())
        {
            return;
        }

        QStringList paths;
        const QModelIndexList selectedIndexes = m_folderTreeView->selectionModel()->selectedRows();
        for (const QModelIndex& index : selectedIndexes)
        {
            const QModelIndex sourceIndex = ToSourceIndex(index);
            if (!sourceIndex.isValid())
            {
                continue;
            }

            const QString path = QDir::cleanPath(m_folderModel->filePath(sourceIndex));
            if (!path.isEmpty() && !paths.contains(path))
            {
                paths.push_back(path);
            }
        }

        DeletePaths(m_folderModel, paths);
    }

    void ContentDrawerWidget::DeleteSelectedContentIndexes()
    {
        if (!m_contentListView || !m_contentListView->selectionModel())
        {
            return;
        }

        QStringList paths;
        const QModelIndexList selectedIndexes = m_contentListView->selectionModel()->selectedIndexes();
        for (const QModelIndex& index : selectedIndexes)
        {
            const QModelIndex sourceIndex = ToSourceIndex(index);
            if (!sourceIndex.isValid())
            {
                continue;
            }

            const QString path = QDir::cleanPath(m_contentModel->filePath(sourceIndex));
            if (!path.isEmpty() && !paths.contains(path))
            {
                paths.push_back(path);
            }
        }

        DeletePaths(m_contentModel, paths);

        if (m_onAssetSelected)
        {
            m_onAssetSelected(QString{});
        }
    }

    void ContentDrawerWidget::DeletePaths(QFileSystemModel* model, const QStringList& paths)
    {
        if (!model || paths.isEmpty())
        {
            return;
        }

        QStringList sortedPaths = paths;
        std::sort(sortedPaths.begin(), sortedPaths.end(), [](const QString& left, const QString& right)
        {
            const int leftDepth = left.count(QDir::separator());
            const int rightDepth = right.count(QDir::separator());
            if (leftDepth != rightDepth)
            {
                return leftDepth > rightDepth;
            }

            return left.size() > right.size();
        });

        for (const QString& path : sortedPaths)
        {
            if (m_onAssetDeleted)
            {
                m_onAssetDeleted(path);
            }

            const QModelIndex index = model->index(path);
            if (index.isValid())
            {
                model->remove(index);
            }
        }
    }

    bool ContentDrawerWidget::IsSceneIndex(const QModelIndex& index) const
    {
        const QModelIndex sourceIndex = ToSourceIndex(index);
        return sourceIndex.isValid() && !m_contentModel->isDir(sourceIndex) && IsSceneFilePath(m_contentModel->filePath(sourceIndex));
    }

    QModelIndex ContentDrawerWidget::ToSourceIndex(const QModelIndex& index) const
    {
        if (!index.isValid())
        {
            return {};
        }

        if (index.model() == m_folderProxyModel)
        {
            return m_folderProxyModel->mapToSource(index);
        }

        if (index.model() == m_contentProxyModel)
        {
            return m_contentProxyModel->mapToSource(index);
        }

        return index;
    }
} // namespace NexusEditor
