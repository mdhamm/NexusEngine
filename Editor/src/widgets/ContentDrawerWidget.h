#pragma once

#include <QStringList>
#include <QWidget>

#include <functional>

class QFileSystemModel;
class QHBoxLayout;
class QListView;
class QModelIndex;
class QPushButton;
class QSortFilterProxyModel;
class QTreeView;
class QWidget;

namespace NexusEditor
{
    class ContentDrawerWidget final : public QWidget
    {
    public:
        /// <summary>
        /// Creates the content drawer rooted at the supplied project directory.
        /// </summary>
        /// <param name="contentRootPath">Absolute project content root.</param>
        /// <param name="parent">Optional parent widget.</param>
        explicit ContentDrawerWidget(const QString& contentRootPath, QWidget* parent = nullptr);

        /// <summary>
        /// Sets the callback invoked when a scene asset is opened from the content drawer.
        /// </summary>
        /// <param name="callback">Callback receiving the opened scene file path.</param>
        void SetSceneOpenedCallback(std::function<void(const QString&)> callback);

        /// <summary>
        /// Sets the callback invoked when an asset is selected in the content drawer.
        /// </summary>
        /// <param name="callback">Callback receiving the selected asset path, or an empty string when selection clears.</param>
        void SetAssetSelectedCallback(std::function<void(const QString&)> callback);

        /// <summary>
        /// Sets the callback invoked when an asset is renamed or moved from the content drawer.
        /// </summary>
        /// <param name="callback">Callback receiving the old and new absolute asset paths.</param>
        void SetAssetRenamedCallback(std::function<void(const QString&, const QString&)> callback);

        /// <summary>
        /// Sets the callback invoked before an asset path or folder is deleted from the content drawer.
        /// </summary>
        /// <param name="callback">Callback receiving the absolute asset path or folder path being deleted.</param>
        void SetAssetDeletedCallback(std::function<void(const QString&)> callback);

    private:
        void SetCurrentFolder(const QString& folderPath);
        void RebuildBreadcrumbs(const QString& folderPath);
        void ShowFolderContextMenu(const QPoint& position);
        void ShowContentContextMenu(const QPoint& position);
        void CreateFolderInDirectory(const QString& directoryPath);
        void CreateSceneInDirectory(const QString& directoryPath);
        void CreateMaterialInDirectory(const QString& directoryPath);
        QString GetNextFolderPath(const QString& directoryPath) const;
        QString GetNextSceneFilePath(const QString& directoryPath) const;
        QString GetNextMaterialFilePath(const QString& directoryPath) const;
        void RenameIndex(const QModelIndex& index);
        void DeleteIndex(const QModelIndex& index);
        void DeleteSelectedFolderIndexes();
        void DeleteSelectedContentIndexes();
        void DeletePaths(QFileSystemModel* model, const QStringList& paths);
        bool IsSceneIndex(const QModelIndex& index) const;
        QModelIndex ToSourceIndex(const QModelIndex& index) const;

        QString m_contentRootPath;
        QFileSystemModel* m_folderModel = nullptr;
        QFileSystemModel* m_contentModel = nullptr;
        QSortFilterProxyModel* m_folderProxyModel = nullptr;
        QSortFilterProxyModel* m_contentProxyModel = nullptr;
        QTreeView* m_folderTreeView = nullptr;
        QWidget* m_breadcrumbWidget = nullptr;
        QHBoxLayout* m_breadcrumbLayout = nullptr;
        QPushButton* m_projectRootButton = nullptr;
        QListView* m_contentListView = nullptr;
        QString m_currentFolderPath;
        std::function<void(const QString&)> m_onSceneOpened;
        std::function<void(const QString&)> m_onAssetSelected;
        std::function<void(const QString&, const QString&)> m_onAssetRenamed;
        std::function<void(const QString&)> m_onAssetDeleted;
    };
} // namespace NexusEditor
