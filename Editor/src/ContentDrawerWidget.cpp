#include "ContentDrawerWidget.h"

#include <QFileSystemModel>
#include <QTreeView>
#include <QVBoxLayout>

namespace NexusEditor
{
    ContentDrawerWidget::ContentDrawerWidget(const QString& contentRootPath, QWidget* parent)
        : QWidget(parent)
    {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        m_fileModel = new QFileSystemModel(this);
        m_fileModel->setRootPath(contentRootPath);

        m_treeView = new QTreeView(this);
        m_treeView->setModel(m_fileModel);
        m_treeView->setRootIndex(m_fileModel->index(contentRootPath));
        m_treeView->setAlternatingRowColors(true);
        m_treeView->setSortingEnabled(true);
        m_treeView->sortByColumn(0, Qt::AscendingOrder);
        layout->addWidget(m_treeView);
    }
} // namespace NexusEditor
