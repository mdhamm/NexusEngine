#include "EditorWindow.h"

#include "ContentDrawerWidget.h"
#include "SceneGraphWidget.h"
#include "SceneViewWidget.h"

#include <QDockWidget>
#include <QStatusBar>
#include <QString>

#ifndef NEXUS_EDITOR_CONTENT_ROOT
#define NEXUS_EDITOR_CONTENT_ROOT "."
#endif

namespace NexusEditor
{
    EditorWindow::EditorWindow(QWidget* parent)
        : QMainWindow(parent)
    {
        setWindowTitle(QStringLiteral("Nexus Editor"));
        resize(1600, 900);
        setDockNestingEnabled(true);
        BuildLayout();
        statusBar()->showMessage(QStringLiteral("Ready"));
    }

    void EditorWindow::BuildLayout()
    {
        m_sceneView = new SceneViewWidget(this);
        setCentralWidget(m_sceneView);

        auto* sceneGraphDock = new QDockWidget(QStringLiteral("Scene Graph"), this);
        sceneGraphDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        m_sceneGraph = new SceneGraphWidget(m_sceneView, sceneGraphDock);
        sceneGraphDock->setWidget(m_sceneGraph);
        addDockWidget(Qt::LeftDockWidgetArea, sceneGraphDock);

        auto* contentDrawerDock = new QDockWidget(QStringLiteral("Content Drawer"), this);
        contentDrawerDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
        m_contentDrawer = new ContentDrawerWidget(QString::fromUtf8(NEXUS_EDITOR_CONTENT_ROOT), contentDrawerDock);
        contentDrawerDock->setWidget(m_contentDrawer);
        addDockWidget(Qt::BottomDockWidgetArea, contentDrawerDock);
    }
} // namespace NexusEditor
