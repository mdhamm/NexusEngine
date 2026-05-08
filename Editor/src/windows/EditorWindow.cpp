#include "EditorWindow.h"

#include "EditorSceneApp.h"
#include "EditorMaterialSerializer.h"
#include "EditorSceneSerializer.h"
#include "widgets/ContentDrawerWidget.h"
#include "widgets/PropertyWidget.h"
#include "widgets/SceneGraphWidget.h"
#include "widgets/SceneViewWidget.h"

#include <filesystem/AssetReferenceRegistry.h>
#include <components/CameraComponent.h>
#include <components/EditorOnlyComponent.h>
#include <components/FlyCameraComponent.h>
#include <components/RenderMeshComponent.h>
#include <components/TransformComponent.h>
#include <rendering/RenderResourceFactory.h>

#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QMenuBar>
#include <QStatusBar>
#include <QVBoxLayout>

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
    namespace
    {
        std::string ToStdString(const QString& value)
        {
            return value.toStdString();
        }

        QString ToQString(const std::string& value)
        {
            return QString::fromStdString(value);
        }
    }

    EditorWindow::EditorWindow(const EditorProject& project, QWidget* parent)
        : QMainWindow(parent)
        , m_project(project)
    {
        m_inputBackend.SetInputState(&m_engine.GetInputState());
        m_engine.SetInputBackend(&m_inputBackend);

        setWindowTitle(QStringLiteral("Nexus Editor - %1").arg(m_project.m_name));
        resize(1600, 900);
        setDockNestingEnabled(true);
        m_sceneFileReference = NexusEngine::IO::AssetReferenceFromPath(ToStdString(QDir(m_project.m_rootPath).filePath(QStringLiteral("EditorScene.nscene"))));
        BuildMenus();
        BuildLayout();
        EnsureEngineInitialized();
        SetSceneMode(true);
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
        return activeScene ? SaveSceneToFile(*activeScene, filePath) : false;
    }

    bool EditorWindow::LoadScene(const QString& filePath)
    {
        EnsureEngineInitialized();
        if (!m_isEngineInitialized || filePath.isEmpty())
        {
            return false;
        }

        NexusEngine::Scene* activeScene = m_engine.ActiveScene();
        if (!activeScene || !LoadSceneFromFile(*activeScene, filePath))
        {
            return false;
        }

        m_hasLoadedScene = true;
        ConfigureEditorCamera();
        ResolveMaterialAssets();
        return true;
    }

    void EditorWindow::TickEngineFrame(float deltaSeconds)
    {
        EnsureEngineInitialized();
        if (!m_isEngineInitialized)
        {
            return;
        }

        if (m_sceneView)
        {
            ResizeSceneViewport(m_sceneView->width(), m_sceneView->height());
        }

        ResolveMaterialAssets();

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

        auto* sceneModePage = new QWidget(viewportTabs);
        auto* sceneModeLayout = new QVBoxLayout(sceneModePage);
        sceneModeLayout->setContentsMargins(0, 0, 0, 0);
        viewportTabs->addTab(sceneModePage, QStringLiteral("Scene"));

        auto* gameModePage = new QWidget(viewportTabs);
        auto* gameModeLayout = new QVBoxLayout(gameModePage);
        gameModeLayout->setContentsMargins(0, 0, 0, 0);
        viewportTabs->addTab(gameModePage, QStringLiteral("Game"));

        m_sceneView = new SceneViewWidget(*this, sceneModePage);
        sceneModeLayout->addWidget(m_sceneView);

        connect(viewportTabs, &QTabWidget::currentChanged, this,
            [this, viewportTabs, sceneModePage, sceneModeLayout, gameModePage, gameModeLayout](int index)
            {
                if (!m_sceneView)
                {
                    return;
                }

                QWidget* targetPage = viewportTabs->widget(index) == gameModePage ? gameModePage : sceneModePage;
                QVBoxLayout* targetLayout = targetPage == gameModePage ? gameModeLayout : sceneModeLayout;
                if (m_sceneView->parentWidget() != targetPage)
                {
                    if (QWidget* currentParent = m_sceneView->parentWidget())
                    {
                        if (QLayout* currentLayout = currentParent->layout())
                        {
                            currentLayout->removeWidget(m_sceneView);
                        }
                    }

                    m_sceneView->setParent(targetPage);
                    targetLayout->addWidget(m_sceneView);
                    m_sceneView->show();
                }

                SetSceneMode(targetPage == sceneModePage);
            });

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

                m_sceneFileReference = NexusEngine::IO::AssetReferenceFromPath(ToStdString(sceneFilePath));
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
        m_contentDrawer->SetAssetSelectedCallback(
            [this](const QString& assetPath)
            {
                if (m_propertyWidget)
                {
                    m_propertyWidget->SetSelectedAssetPath(assetPath);
                }
            });
        m_contentDrawer->SetAssetRenamedCallback(
            [this](const QString& oldPath, const QString& newPath)
            {
                UpdateLoadedSceneFilePath(oldPath, newPath);
            });
        contentDrawerDock->setWidget(m_contentDrawer);
        addDockWidget(Qt::BottomDockWidgetArea, contentDrawerDock);
    }

    void EditorWindow::ResolveMaterialAssets()
    {
        NexusEngine::Scene* activeScene = m_engine.ActiveScene();
        if (!activeScene)
        {
            return;
        }

        NexusEngine::RenderResourceFactory* factory = activeScene->GetResourceFactory();
        if (!factory)
        {
            return;
        }

        activeScene->m_world.each<NexusEngine::RenderMeshComponent>(
            [this, factory](flecs::entity, NexusEngine::RenderMeshComponent& renderMesh)
            {
                if (renderMesh.m_materialAssetPath.empty())
                {
                    return;
                }

                const QString configuredPath = QString::fromStdString(renderMesh.m_materialAssetPath).trimmed();
                const QString absolutePath = QDir::cleanPath(QFileInfo(configuredPath).isAbsolute()
                    ? configuredPath
                    : QDir(m_project.m_rootPath).filePath(configuredPath));

                const QFileInfo materialFileInfo(absolutePath);
                if (!materialFileInfo.exists())
                {
                    return;
                }

                MaterialAssetCacheEntry cacheEntry = m_materialAssetCache.value(absolutePath);
                if (!cacheEntry.m_material || cacheEntry.m_lastModified != materialFileInfo.lastModified())
                {
                    MaterialAssetData materialData;
                    if (!LoadMaterialAssetFile(absolutePath, materialData))
                    {
                        return;
                    }

                    const auto toCullMode = [](const QString& cullModeText)
                    {
                        if (cullModeText.compare(QStringLiteral("Front"), Qt::CaseInsensitive) == 0)
                        {
                            return Diligent::CULL_MODE_FRONT;
                        }

                        if (cullModeText.compare(QStringLiteral("Back"), Qt::CaseInsensitive) == 0)
                        {
                            return Diligent::CULL_MODE_BACK;
                        }

                        return Diligent::CULL_MODE_NONE;
                    };

                    std::shared_ptr<NexusEngine::Material> material(
                        factory->CreateSurfaceMaterialFromFiles(
                            materialData.m_name.toUtf8().constData(),
                            materialData.m_vertexShaderPath.toUtf8().constData(),
                            materialData.m_pixelShaderPath.toUtf8().constData(),
                            materialData.m_isTransparent,
                            toCullMode(materialData.m_cullMode),
                            materialData.m_depthTestEnabled,
                            materialData.m_depthWriteEnabled));

                    if (!material)
                    {
                        return;
                    }

                    cacheEntry.m_lastModified = materialFileInfo.lastModified();
                    cacheEntry.m_material = std::move(material);
                    m_materialAssetCache.insert(absolutePath, cacheEntry);
                }

                renderMesh.material = cacheEntry.m_material.get();
            });
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

        m_isEngineInitialized = m_engine.Initialize(nativeWindow, std::move(game), "");
    }

    void EditorWindow::SetSceneMode(bool isSceneMode)
    {
        m_isSceneMode = isSceneMode;

        if (!m_isEngineInitialized)
        {
            return;
        }

        if (NexusEngine::Scene* activeScene = m_engine.ActiveScene())
        {
            if (m_isSceneMode)
            {
                activeScene->m_world.remove<NexusEngine::GameplayEnabled>();
                activeScene->m_world.remove<NexusEngine::PhysicsEnabled>();
            }
            else
            {
                activeScene->m_world.add<NexusEngine::GameplayEnabled>();
                activeScene->m_world.add<NexusEngine::PhysicsEnabled>();
            }
        }

        ConfigureEditorCamera();

        if (m_sceneGraph)
        {
            m_sceneGraph->Refresh();
        }

        if (m_propertyWidget)
        {
            m_propertyWidget->Refresh();
        }
    }

    void EditorWindow::ConfigureEditorCamera()
    {
        NexusEngine::Scene* activeScene = m_engine.ActiveScene();
        if (!activeScene)
        {
            return;
        }

        flecs::entity editorCamera = activeScene->m_world.lookup("EditorCamera");
        if (!m_hasLoadedScene)
        {
            if (editorCamera.is_valid())
            {
                editorCamera.destruct();
            }
            return;
        }

        if (!editorCamera.is_valid())
        {
            editorCamera = activeScene->m_world.entity("EditorCamera")
                .add<NexusEngine::EditorOnlyComponent>()
                .set(NexusEngine::TransformComponent::FromLocal(
                    Diligent::float3(0.0f, 0.0f, 10.0f),
                    NexusEngine::Quaternion::FromEuler(0.0f, 0.0f, 0.0f),
                    Diligent::float3(1.0f, 1.0f, 1.0f)));
        }
        else if (!editorCamera.has<NexusEngine::EditorOnlyComponent>())
        {
            editorCamera.add<NexusEngine::EditorOnlyComponent>();
        }

        if (m_isSceneMode)
        {
            if (!editorCamera.has<NexusEngine::CameraComponent>())
            {
                editorCamera.set(NexusEngine::CameraComponent{});
            }

            if (auto* camera = editorCamera.get_mut<NexusEngine::CameraComponent>())
            {
                camera->m_target = NexusEngine::CameraComponent::Target::SwapChain;
                camera->m_priority = 1000;
            }

            if (!editorCamera.has<NexusEngine::FlyCameraComponent>())
            {
                editorCamera.set(NexusEngine::FlyCameraComponent{});
            }
        }
        else
        {
            if (editorCamera.has<NexusEngine::FlyCameraComponent>())
            {
                editorCamera.remove<NexusEngine::FlyCameraComponent>();
            }

            if (editorCamera.has<NexusEngine::CameraComponent>())
            {
                editorCamera.remove<NexusEngine::CameraComponent>();
            }
        }
    }

    void EditorWindow::UpdateLoadedSceneFilePath(const QString& oldPath, const QString& newPath)
    {
        if (m_sceneFileReference.IsEmpty())
        {
            return;
        }

        std::string currentPathText;
        if (!NexusEngine::IO::ResolveAssetReferencePath(m_sceneFileReference, ToStdString(m_project.m_rootPath), currentPathText))
        {
            return;
        }

        const QString currentPath = QDir::cleanPath(ToQString(currentPathText));
        if (currentPath.compare(QDir::cleanPath(newPath), Qt::CaseInsensitive) == 0
            && currentPath.compare(QDir::cleanPath(oldPath), Qt::CaseInsensitive) != 0)
        {
            statusBar()->showMessage(QStringLiteral("Scene path updated to %1").arg(currentPath), 3000);
        }
    }

    bool EditorWindow::ResolveSceneFilePath()
    {
        std::string resolvedPath;
        return NexusEngine::IO::ResolveAssetReferencePath(m_sceneFileReference, ToStdString(m_project.m_rootPath), resolvedPath);
    }

    void EditorWindow::SaveScene()
    {
        if (m_sceneFileReference.IsEmpty())
        {
            SaveSceneAs();
            return;
        }

        std::string resolvedPath;
        if (!NexusEngine::IO::ResolveAssetReferencePath(m_sceneFileReference, ToStdString(m_project.m_rootPath), resolvedPath))
        {
            statusBar()->showMessage(QStringLiteral("Current scene file could not be found. Use Save Scene As..."), 3000);
            return;
        }

        const QString scenePath = ToQString(resolvedPath);
        if (SaveActiveScene(scenePath, ToQString(m_sceneFileReference.GetGuid())))
        {
            statusBar()->showMessage(QStringLiteral("Saved scene to %1").arg(scenePath), 3000);
        }
        else
        {
            statusBar()->showMessage(QStringLiteral("Failed to save scene"), 3000);
        }
    }

    void EditorWindow::SaveSceneAs()
    {
        const std::string currentPath = NexusEngine::IO::GetAssetReferencePath(m_sceneFileReference, ToStdString(m_project.m_rootPath));
        const QString filePath = QFileDialog::getSaveFileName(
            this,
            QStringLiteral("Save Scene"),
            m_sceneFileReference.IsEmpty() ? QStringLiteral("EditorScene.nscene") : ToQString(currentPath),
            QStringLiteral("Nexus Scene Files (*.nscene);;All Files (*.*)"));

        if (filePath.isEmpty())
        {
            return;
        }

        NexusEngine::IO::AssetReference reference = NexusEngine::IO::AssetReferenceFromPath(ToStdString(filePath));
        if (reference.IsEmpty())
        {
            reference = NexusEngine::IO::CreateAssetReference(ToStdString(filePath));
        }

        if (reference.IsEmpty())
        {
            statusBar()->showMessage(QStringLiteral("Failed to create scene asset reference"), 3000);
            return;
        }

        m_sceneFileReference = reference;

        if (SaveActiveScene(filePath, ToQString(m_sceneFileReference.GetGuid())))
        {
            statusBar()->showMessage(QStringLiteral("Saved scene to %1").arg(filePath), 3000);
        }
        else
        {
            statusBar()->showMessage(QStringLiteral("Failed to save scene"), 3000);
        }
    }
} // namespace NexusEditor
