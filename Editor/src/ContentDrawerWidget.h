#pragma once

#include <QWidget>

class QFileSystemModel;
class QTreeView;

namespace NexusEditor
{
    class ContentDrawerWidget final : public QWidget
    {
    public:
        /// <summary>
        /// Creates the content drawer rooted at the supplied directory.
        /// </summary>
        /// <param name="contentRootPath">Workspace-relative or absolute content root.</param>
        /// <param name="parent">Optional parent widget.</param>
        explicit ContentDrawerWidget(const QString& contentRootPath, QWidget* parent = nullptr);

    private:
        QFileSystemModel* m_fileModel = nullptr;
        QTreeView* m_treeView = nullptr;
    };
} // namespace NexusEditor
