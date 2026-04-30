#include "EditorWindow.h"

#include "ContentDrawerWidget.h"
#include "PropertyWidget.h"
#include "SceneGraphWidget.h"
#include "SceneViewWidget.h"

#include <QAction>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QString>

#include <cstdint>

#ifndef NEXUS_EDITOR_CONTENT_ROOT
#define NEXUS_EDITOR_CONTENT_ROOT "."
#endif

namespace NexusEditor
{
    EditorWindow::EditorWindow(const EditorProject& project, QWidget* parent)
        : QMainWindow(parent)
        , m_project(project)
    {
        setWindowTitle(QStringLiteral("Nexus Editor - %1").arg(m_project.m_name));
        resize(1600, 900);
        setDockNestingEnabled(true);
        m_sceneFileReference.BindToPath(QDir(m_project.m_rootPath).filePath(QStringLiteral("EditorScene.nscene")));
        BuildMenus();
        BuildLayout();
        statusBar()->showMessage(QStringLiteral("Ready - %1").arg(m_project.m_rootPath));
    }

    void EditorWindow::BuildMenus()
    {
        auto* fileMenu = menuBar()->addMenu(QStringLiteral("&File"));

        auto* saveAction = fileMenu->addAction(QStringLiteral("&Save Scene"));
        connect(saveAction, &QAction::triggered, this, [this]() { SaveScene(); });

        auto* saveAsAction = fileMenu->addAction(QStringLiteral("Save Scene &As..."));
        connect(saveAsAction, &QAction::triggered, this, [this]() { SaveSceneAs(); });
    }

    void EditorWindow::BuildLayout()
    {
        auto* viewportTabs = new QTabWidget(this);
        viewportTabs->setDocumentMode(true);
        viewportTabs->setMovable(true);
        setCentralWidget(viewportTabs);

        m_sceneView = new SceneViewWidget(viewportTabs);
        viewportTabs->addTab(m_sceneView, QStringLiteral("Scene"));

        auto* gameView = new QWidget(viewportTabs);
        viewportTabs->addTab(gameView, QStringLiteral("Game"));

        auto* sceneGraphDock = new QDockWidget(QStringLiteral("Scene Graph"), this);
        sceneGraphDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        m_sceneGraph = new SceneGraphWidget(m_sceneView, sceneGraphDock);
        sceneGraphDock->setWidget(m_sceneGraph);
        addDockWidget(Qt::LeftDockWidgetArea, sceneGraphDock);

        auto* propertyDock = new QDockWidget(QStringLiteral("Properties"), this);
        propertyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        m_propertyWidget = new PropertyWidget(m_sceneView, propertyDock);
        propertyDock->setWidget(m_propertyWidget);
        addDockWidget(Qt::RightDockWidgetArea, propertyDock);

        m_sceneView->SetSceneReadyCallback(
            [this]()
            {
                if (m_sceneGraph)
                {
                    m_sceneGraph->Refresh();
                }

                if (m_propertyWidget)
                {
                    m_propertyWidget->Refresh();
                }
            });

        m_sceneGraph->SetSelectionChangedCallback(
            [this](std::uint64_t entityId)
            {
                if (m_propertyWidget)
                {
                    m_propertyWidget->SetSelectedEntityId(entityId);
                }
            });

        auto* contentDrawerDock = new QDockWidget(QStringLiteral("Content Drawer"), this);
        contentDrawerDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
        m_contentDrawer = new ContentDrawerWidget(m_project.m_rootPath, contentDrawerDock);
        m_contentDrawer->SetSceneOpenedCallback(
            [this](const QString& sceneFilePath)
            {
                if (!m_sceneView || !m_sceneView->LoadScene(sceneFilePath))
                {
                    statusBar()->showMessage(QStringLiteral("Failed to load scene %1").arg(sceneFilePath), 3000);
                    return;
                }

                m_sceneFileReference.BindToPath(sceneFilePath);
                if (m_sceneGraph)
                {
                    m_sceneGraph->Refresh();
                }

                if (m_propertyWidget)
                {
                    m_propertyWidget->SetSelectedEntityId(0);
                    m_propertyWidget->Refresh();
                }

                statusBar()->showMessage(QStringLiteral("Loaded scene %1").arg(sceneFilePath), 3000);
            });
        m_contentDrawer->SetAssetRenamedCallback(
            [this](const QString& oldPath, const QString& newPath)
            {
                UpdateLoadedSceneFilePath(oldPath, newPath);
            });
        contentDrawerDock->setWidget(m_contentDrawer);
        addDockWidget(Qt::BottomDockWidgetArea, contentDrawerDock);
    }

    void EditorWindow::UpdateLoadedSceneFilePath(const QString& oldPath, const QString& newPath)
    {
        if (m_sceneFileReference.IsEmpty())
        {
            return;
        }

        const QString currentPath = QDir::cleanPath(m_sceneFileReference.GetPath());
        if (currentPath.compare(QDir::cleanPath(newPath), Qt::CaseInsensitive) == 0
            && currentPath.compare(QDir::cleanPath(oldPath), Qt::CaseInsensitive) != 0)
        {
            statusBar()->showMessage(QStringLiteral("Scene path updated to %1").arg(m_sceneFileReference.GetPath()), 3000);
        }
    }

    bool EditorWindow::ResolveSceneFilePath()
    {
        return m_sceneFileReference.ResolveWithinProject(m_project.m_rootPath);
    }

    void EditorWindow::SaveScene()
    {
        if (m_sceneFileReference.IsEmpty())
        {
            SaveSceneAs();
            return;
        }

        if (!ResolveSceneFilePath())
        {
            statusBar()->showMessage(QStringLiteral("Current scene file could not be found. Use Save Scene As..."), 3000);
            return;
        }

        if (m_sceneView && m_sceneView->SaveActiveScene(m_sceneFileReference.GetPath(), m_sceneFileReference.GetGuid()))
        {
            statusBar()->showMessage(QStringLiteral("Saved scene to %1").arg(m_sceneFileReference.GetPath()), 3000);
        }
        else
        {
            statusBar()->showMessage(QStringLiteral("Failed to save scene"), 3000);
        }
    }

    void EditorWindow::SaveSceneAs()
    {
        const QString filePath = QFileDialog::getSaveFileName(
            this,
            QStringLiteral("Save Scene"),
            m_sceneFileReference.IsEmpty() ? QStringLiteral("EditorScene.nscene") : m_sceneFileReference.GetPath(),
            QStringLiteral("Nexus Scene Files (*.nscene);;All Files (*.*)"));

        if (filePath.isEmpty())
        {
            return;
        }

        m_sceneFileReference.BindToPath(filePath);

        if (m_sceneView && m_sceneView->SaveActiveScene(m_sceneFileReference.GetPath(), m_sceneFileReference.GetGuid()))
        {
            statusBar()->showMessage(QStringLiteral("Saved scene to %1").arg(m_sceneFileReference.GetPath()), 3000);
        }
        else
        {
            statusBar()->showMessage(QStringLiteral("Failed to save scene"), 3000);
        }
    }
} // namespace NexusEditor
