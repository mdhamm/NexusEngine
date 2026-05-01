#include "EditorWindow.h"

#include "EditorSceneApp.h"
#include "EditorSceneSerializer.h"
#include "widgets/ContentDrawerWidget.h"
#include "widgets/PropertyWidget.h"
#include "widgets/SceneGraphWidget.h"
#include "widgets/SceneViewWidget.h"

#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QMenuBar>
#include <QStatusBar>
#include <QTabWidget>

#ifdef emit
#undef emit
#endif

#include <Game.h>

#include <algorithm>
#include <cstdint>
#include <memory>

#ifndef NEXUS_EDITOR_CONTENT_ROOT
#define NEXUS_EDITOR_CONTENT_ROOT "."
#endif

namespace NexusEditor
{
    EditorWindow::EditorWindow(const EditorProject& project, QWidget* parent)
        : QMainWindow(parent)
        , m_project(project)
    {
        m_inputBackend.SetInputState(&m_engine.GetInputState());
        m_engine.SetInputBackend(&m_inputBackend);

        setWindowTitle(QStringLiteral("Nexus Editor - %1").arg(m_project.m_name));
        resize(1600, 900);
        setDockNestingEnabled(true);
        m_sceneFileReference.BindToPath(QDir(m_project.m_rootPath).filePath(QStringLiteral("EditorScene.nscene")));
        BuildMenus();
        BuildLayout();
        EnsureEngineInitialized();
        statusBar()->showMessage(QStringLiteral("Ready - %1").arg(m_project.m_rootPath));
    }

    NexusEngine::Engine* EditorWindow::GetEngine()
    {
        return &m_engine;
    }

    NexusEngine::Scene* EditorWindow::GetActiveScene()
    {
        return m_engine.ActiveScene();
    }

    bool EditorWindow::IsEngineInitialized() const
    {
        return m_isEngineInitialized;
    }

    bool EditorWindow::SaveActiveScene(const QString& filePath, const QString& assetGuid) const
    {
        if (!m_isEngineInitialized || filePath.isEmpty())
        {
            return false;
        }

        NexusEngine::Scene* activeScene = const_cast<NexusEngine::Engine&>(m_engine).ActiveScene();
        return activeScene ? SaveSceneToFile(*activeScene, filePath, assetGuid) : false;
    }

    bool EditorWindow::LoadScene(const QString& filePath)
    {
        EnsureEngineInitialized();
        if (!m_isEngineInitialized || filePath.isEmpty())
        {
            return false;
        }

        NexusEngine::Scene* activeScene = m_engine.ActiveScene();
        return activeScene ? LoadSceneFromFile(*activeScene, filePath) : false;
    }

    void EditorWindow::TickEngineFrame(float deltaSeconds)
    {
        EnsureEngineInitialized();
        if (!m_isEngineInitialized)
        {
            return;
        }

        m_engine.RunFrame(deltaSeconds);

        if (m_propertyWidget)
        {
            m_propertyWidget->RefreshIfNotInteracting();
        }
    }

    QtInputBackend& EditorWindow::GetInputBackend()
    {
        return m_inputBackend;
    }

    void EditorWindow::ResizeSceneViewport(int width, int height)
    {
        EnsureEngineInitialized();
        if (!m_isEngineInitialized)
        {
            return;
        }

        m_engine.ResizeOutput(width, height);
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

        m_sceneView = new SceneViewWidget(*this, viewportTabs);
        viewportTabs->addTab(m_sceneView, QStringLiteral("Scene"));

        auto* gameView = new QWidget(viewportTabs);
        viewportTabs->addTab(gameView, QStringLiteral("Game"));

        auto* sceneGraphDock = new QDockWidget(QStringLiteral("Scene Graph"), this);
        sceneGraphDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        m_sceneGraph = new SceneGraphWidget(*this, sceneGraphDock);
        sceneGraphDock->setWidget(m_sceneGraph);
        addDockWidget(Qt::LeftDockWidgetArea, sceneGraphDock);

        auto* propertyDock = new QDockWidget(QStringLiteral("Properties"), this);
        propertyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        m_propertyWidget = new PropertyWidget(*this, propertyDock);
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
                if (!LoadScene(sceneFilePath))
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

    void EditorWindow::EnsureEngineInitialized()
    {
        if (m_isEngineInitialized || !m_sceneView)
        {
            return;
        }

        SampleGame::RegisterEditorComponentDescriptors();

        std::unique_ptr<NexusEngine::IGameApp> game = std::make_unique<EditorSceneApp>();
        if (!game)
        {
            return;
        }

        NexusEngine::NativeWindow nativeWindow{};
        nativeWindow.m_width = std::max(1, m_sceneView->width());
        nativeWindow.m_height = std::max(1, m_sceneView->height());
        nativeWindow.m_hWnd = reinterpret_cast<void*>(m_sceneView->winId());

        m_isEngineInitialized = m_engine.Initialize(nativeWindow, std::move(game));
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

        if (SaveActiveScene(m_sceneFileReference.GetPath(), m_sceneFileReference.GetGuid()))
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

        if (SaveActiveScene(m_sceneFileReference.GetPath(), m_sceneFileReference.GetGuid()))
        {
            statusBar()->showMessage(QStringLiteral("Saved scene to %1").arg(m_sceneFileReference.GetPath()), 3000);
        }
        else
        {
            statusBar()->showMessage(QStringLiteral("Failed to save scene"), 3000);
        }
    }
} // namespace NexusEditor
