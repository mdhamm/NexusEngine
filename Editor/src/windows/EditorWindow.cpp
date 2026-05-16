#include "EditorWindow.h"

#include "EditorSceneApp.h"
#include "EditorSceneSerializer.h"
#include "widgets/ContentDrawerWidget.h"
#include "widgets/PropertyWidget.h"
#include "widgets/SceneGraphWidget.h"
#include "widgets/SceneViewWidget.h"

#include <assets/MaterialAsset.h>
#include <filesystem/AssetReferenceRegistry.h>
#include <filesystem/FileIO.h>
#include <components/CameraComponent.h>
#include <components/EditorOnlyComponent.h>
#include <components/FlyCameraComponent.h>
#include <components/RenderMeshComponent.h>
#include <components/TransformComponent.h>
#include <rendering/RenderResourceFactory.h>

#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QVBoxLayout>

#ifdef emit
#undef emit
#endif

#include <Game.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
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
        BuildMenus();
        BuildLayout();
        InitializeEngine();
        if (NexusEngine::IO::AssetReferenceRegistry* assetReferenceRegistry = m_engine.GetAssetReferenceRegistry())
        {
            m_sceneFileReference = assetReferenceRegistry->GetOrCreateAssetReferenceByPath(
                std::filesystem::path(QDir(m_project.m_rootPath).filePath(QStringLiteral("EditorScene.nscene")).toStdWString()));
        }
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

    bool EditorWindow::SaveActiveScene(const QString& filePath, const QString& assetGuid) const
    {
        REF(assetGuid);
        if (filePath.isEmpty())
        {
            return false;
        }

        NexusEngine::Scene* activeScene = const_cast<NexusEngine::Engine&>(m_engine).ActiveScene();
        return activeScene ? SaveSceneToFile(*activeScene, filePath) : false;
    }

    bool EditorWindow::LoadScene(const std::filesystem::path& sceneFilePath)
    {
        if (sceneFilePath.empty())
        {
            return false;
        }

        NexusEngine::Scene* activeScene = m_engine.ActiveScene();
        assert(activeScene);
        if (!activeScene)
        {
            return false;
        }

        std::vector<flecs::entity> entitiesToDestroy;
        activeScene->GetRootEntity().children(
            [&](flecs::entity entity)
            {
                if (!entity.is_valid() || !entity.is_alive() || !entity.has<NexusEngine::TransformComponent>())
                {
                    return;
                }

                if (entity.has<NexusEngine::EditorOnlyComponent>())
                {
                    return;
                }

                entitiesToDestroy.push_back(entity);
            });

        for (const flecs::entity& entity : entitiesToDestroy)
        {
            activeScene->DestroyEntity(entity);
        }

        if (!LoadSceneFromFile(*activeScene, QString::fromStdWString(sceneFilePath.wstring())))
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

    void EditorWindow::AddInspectedTargetChangedListener(std::function<void(const InspectedTarget&)> listener)
    {
        if (!listener)
        {
            return;
        }

        m_inspectedTargetChangedListeners.push_back(std::move(listener));
        m_inspectedTargetChangedListeners.back()(m_inspectedTarget);
    }

    void EditorWindow::ResizeSceneViewport(int width, int height)
    {
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
        // TODO: A more scalable implementation is needed to handle more widgets.

        // Viewport Tabs. e.g. Scene/Game view modes
        auto* viewportTabs = new QTabWidget(this);
        viewportTabs->setDocumentMode(true);
        viewportTabs->setMovable(true);
        setCentralWidget(viewportTabs);

        // Scene View
        auto* sceneModePage = new QWidget(viewportTabs);
        auto* sceneModeLayout = new QVBoxLayout(sceneModePage);
        sceneModeLayout->setContentsMargins(0, 0, 0, 0);
        viewportTabs->addTab(sceneModePage, QStringLiteral("Scene"));

        // Game View (for testing gameplay mode)
        auto* gameModePage = new QWidget(viewportTabs);
        auto* gameModeLayout = new QVBoxLayout(gameModePage);
        gameModeLayout->setContentsMargins(0, 0, 0, 0);
        viewportTabs->addTab(gameModePage, QStringLiteral("Game"));

        // Start in scene view mode by default
        // TODO: Revisit this at a later time. It would be nice to be able to look at both Scene and Game views at the same time.
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

                // TODO: This is incorrect. game view should be running the editor scene's active camera, but with gameplay systems enabled. This will require a more robust way to manage editor vs gameplay modes in the scene.
                SetSceneMode(targetPage == sceneModePage);
            });

        // Scene Graph
        auto* sceneGraphDock = new QDockWidget(QStringLiteral("Scene Graph"), this);
        sceneGraphDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        m_sceneGraph = new SceneGraphWidget(*this, sceneGraphDock);
        sceneGraphDock->setWidget(m_sceneGraph);
        addDockWidget(Qt::LeftDockWidgetArea, sceneGraphDock);

        // Property Editor
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

        // Change contents of property editor when element is selected in scene graph
        m_sceneGraph->SetSelectionChangedCallback(
            [this](std::uint64_t entityId)
            {
                InspectedTarget inspectedTarget;
                if (entityId != 0)
                {
                    inspectedTarget.m_value = EntityInspectedTarget{ entityId };
                }

                SetInspectedTarget(inspectedTarget);
            });

        // Content Drawer
        auto* contentDrawerDock = new QDockWidget(QStringLiteral("Content Drawer"), this);
        contentDrawerDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
        m_contentDrawer = new ContentDrawerWidget(*this, m_project.m_rootPath, contentDrawerDock);
        m_contentDrawer->SetSceneOpenedCallback(
            [this](const QString& sceneFilePath)
            {
                const std::string strSceneFilePath = sceneFilePath.toStdString();

                const QString requestedScenePath = QDir::cleanPath(sceneFilePath);
                const QString currentScenePath = ResolveSceneFilePath();
                if (!currentScenePath.isEmpty() &&
                    currentScenePath.compare(requestedScenePath, Qt::CaseInsensitive) == 0)
                {
                    return;
                }

                if (IsCurrentSceneDirty())
                {
                    const QMessageBox::StandardButton result = QMessageBox::question(
                        this,
                        QStringLiteral("Unsaved Changes"),
                        QStringLiteral("Save changes to the current scene before opening %1?").arg(QFileInfo(requestedScenePath).fileName()),
                        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                        QMessageBox::Yes);

                    if (result == QMessageBox::Cancel)
                    {
                        return;
                    }

                    if (result == QMessageBox::Yes && !SaveScene())
                    {
                        return;
                    }
                }

                if (!LoadScene(strSceneFilePath))
                {
                    statusBar()->showMessage(QStringLiteral("Failed to load scene %1").arg(sceneFilePath), 3000);
                    return;
                }

                NexusEngine::IO::AssetReferenceRegistry* assetReferenceRegistry = m_engine.GetAssetReferenceRegistry();
                assert(assetReferenceRegistry);
                if (!assetReferenceRegistry)
                {
                    return;
                }
                const std::filesystem::path absoluteSceneFilePath(sceneFilePath.toStdWString());
                NexusEngine::IO::AssetReference assetReference = assetReferenceRegistry->GetOrCreateAssetReferenceByPath(absoluteSceneFilePath);
                assert(!assetReference.IsEmpty());
                
                m_sceneFileReference = assetReference;
                m_lastSavedSceneSnapshot = CaptureActiveSceneSnapshot();
                if (m_sceneGraph)
                {
                    m_sceneGraph->Refresh();
                }

                if (m_propertyWidget)
                {
                    m_propertyWidget->Refresh();
                }

                SetInspectedTarget(InspectedTarget{});

                statusBar()->showMessage(QStringLiteral("Loaded scene %1").arg(sceneFilePath), 3000);
            });
        m_contentDrawer->SetAssetSelectedCallback(
            [this](const QString& assetPath)
            {
                if (m_propertyWidget)
                {
                    if (m_propertyWidget->TryAssignPickedAssetPath(assetPath))
                    {
                        return;
                    }
                }

                InspectedTarget inspectedTarget;
                const QString cleanAssetPath = assetPath.trimmed();
                if (!cleanAssetPath.isEmpty())
                {
                    inspectedTarget.m_value = AssetInspectedTarget{ cleanAssetPath };
                }

                SetInspectedTarget(inspectedTarget);
            });
        m_contentDrawer->SetAssetRenamedCallback(
            [this](const QString& oldPath, const QString& newPath)
            {
                NexusEngine::IO::AssetReferenceRegistry* assetReferenceRegistry = m_engine.GetAssetReferenceRegistry();
                assert(assetReferenceRegistry);
                if (!assetReferenceRegistry)
                {
                    return;
                }

                NexusEngine::IO::AssetReference assetReference = assetReferenceRegistry->GetOrCreateAssetReferenceByPath(oldPath.toStdString());
                if (assetReference.IsEmpty())
                {
                    return;
                }

                assetReferenceRegistry->UpdateAssetReferencePath(assetReference, newPath.toStdString());
            });
        m_contentDrawer->SetAssetDeletedCallback(
            [this](const QString& deletedPath)
            {
                NexusEngine::IO::AssetReferenceRegistry* assetReferenceRegistry = m_engine.GetAssetReferenceRegistry();
                assert(assetReferenceRegistry);
                if (!assetReferenceRegistry)
                {
                    return;
                }

                assetReferenceRegistry->RemoveAssetReferencesForPath(std::filesystem::path(deletedPath.toStdWString()));
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

        activeScene->GetWorld().each<NexusEngine::RenderMeshComponent>(
            [this, activeScene, factory](flecs::entity entity, NexusEngine::RenderMeshComponent& renderMesh)
            {
                if (!activeScene->ContainsEntity(entity))
                {
                    return;
                }

                if (renderMesh.m_materialAssetReference.IsEmpty())
                {
                    return;
                }

                NexusEngine::IO::AssetReferenceRegistry* assetReferenceRegistry = m_engine.GetAssetReferenceRegistry();
                if (!assetReferenceRegistry)
                {
                    return;
                }

                const std::filesystem::path resolvedPath = assetReferenceRegistry->ResolveAssetReferencePath(renderMesh.m_materialAssetReference);
                if (resolvedPath.empty())
                {
                    return;
                }

                const QString absolutePath = QDir::cleanPath(QDir(m_project.m_rootPath).filePath(QString::fromStdWString(resolvedPath.wstring())));

                const QFileInfo materialFileInfo(absolutePath);
                if (!materialFileInfo.exists())
                {
                    return;
                }

                MaterialAssetCacheEntry cacheEntry = m_materialAssetCache.value(absolutePath);
                if (!cacheEntry.m_material || cacheEntry.m_lastModified != materialFileInfo.lastModified())
                {
                    NexusEngine::MaterialAsset materialData;
                    if (!NexusEngine::LoadMaterialAssetFile(std::filesystem::path(absolutePath.toStdWString()), materialData))
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
                            materialData.m_name.c_str(),
                            materialData.m_vertexShaderPath.c_str(),
                            materialData.m_pixelShaderPath.c_str(),
                            materialData.m_isTransparent,
                            toCullMode(QString::fromStdString(materialData.m_cullMode)),
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

    void EditorWindow::SetInspectedTarget(const InspectedTarget& inspectedTarget)
    {
        if (m_inspectedTarget.m_value == inspectedTarget.m_value)
        {
            return;
        }

        m_inspectedTarget = inspectedTarget;
        for (const std::function<void(const InspectedTarget&)>& listener : m_inspectedTargetChangedListeners)
        {
            if (listener)
            {
                listener(m_inspectedTarget);
            }
        }
    }

    void EditorWindow::InitializeEngine()
    {
        if (!m_sceneView)
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
        m_engine.Initialize(nativeWindow, std::move(game), m_project.m_rootPath.toStdString());

        NexusEngine::Scene& sceneRef = m_engine.CreateScene("EditorScene");
        m_engine.SetActiveScene(sceneRef);
    }

    void EditorWindow::SetSceneMode(bool isSceneMode)
    {
        m_isSceneMode = isSceneMode;

        if (NexusEngine::Scene* activeScene = m_engine.ActiveScene())
        {
            flecs::world& world = activeScene->GetWorld();
            if (m_isSceneMode)
            {
                world.remove<NexusEngine::GameplayEnabled>();
                world.remove<NexusEngine::PhysicsEnabled>();
            }
            else
            {
                world.add<NexusEngine::GameplayEnabled>();
                world.add<NexusEngine::PhysicsEnabled>();
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

        flecs::entity editorCamera = activeScene->FindEntityByName("EditorCamera");
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
            editorCamera = activeScene->CreateEntity("EditorCamera")
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

    QString EditorWindow::ResolveSceneFilePath() const
    {
        NexusEngine::IO::AssetReferenceRegistry* assetReferenceRegistry = const_cast<NexusEngine::Engine&>(m_engine).GetAssetReferenceRegistry();
        if (!assetReferenceRegistry)
        {
            return {};
        }

        const std::filesystem::path resolvedPath = assetReferenceRegistry->ResolveAssetReferencePath(m_sceneFileReference);
        if (resolvedPath.empty())
        {
            return {};
        }

        const QString pathText = QString::fromStdWString(resolvedPath.wstring());
        return QDir::cleanPath(QFileInfo(pathText).isAbsolute()
            ? pathText
            : QDir(m_project.m_rootPath).filePath(pathText));
    }

    std::string EditorWindow::CaptureActiveSceneSnapshot() const
    {
        NexusEngine::Scene* activeScene = const_cast<NexusEngine::Engine&>(m_engine).ActiveScene();
        return activeScene ? SerializeSceneToJsonText(*activeScene) : std::string{};
    }

    bool EditorWindow::IsCurrentSceneDirty() const
    {
        return !m_lastSavedSceneSnapshot.empty() && CaptureActiveSceneSnapshot() != m_lastSavedSceneSnapshot;
    }

    bool EditorWindow::SaveScene()
    {
        if (m_sceneFileReference.IsEmpty())
        {
            return SaveSceneAs();
        }

        NexusEngine::IO::AssetReferenceRegistry* assetReferenceRegistry = m_engine.GetAssetReferenceRegistry();
        if (!assetReferenceRegistry)
        {
            statusBar()->showMessage(QStringLiteral("Current scene file could not be found. Use Save Scene As..."), 3000);
            return false;
        }

        const std::filesystem::path resolvedPath = assetReferenceRegistry->ResolveAssetReferencePath(m_sceneFileReference);
        if (resolvedPath.empty())
        {
            statusBar()->showMessage(QStringLiteral("Current scene file could not be found. Use Save Scene As..."), 3000);
            return false;
        }

        const QString scenePath = ResolveSceneFilePath();
        if (SaveActiveScene(scenePath, QString::fromStdString(m_sceneFileReference.GetGuid())))
        {
            m_lastSavedSceneSnapshot = CaptureActiveSceneSnapshot();
            statusBar()->showMessage(QStringLiteral("Saved scene to %1").arg(scenePath), 3000);
            return true;
        }

        statusBar()->showMessage(QStringLiteral("Failed to save scene"), 3000);
        return false;
    }

    bool EditorWindow::SaveSceneAs()
    {
        NexusEngine::IO::AssetReferenceRegistry* assetReferenceRegistry = m_engine.GetAssetReferenceRegistry();
        const std::filesystem::path currentPath = assetReferenceRegistry
            ? assetReferenceRegistry->ResolveAssetReferencePath(m_sceneFileReference)
            : std::filesystem::path{};
        const QString filePath = QFileDialog::getSaveFileName(
            this,
            QStringLiteral("Save Scene"),
            m_sceneFileReference.IsEmpty() || currentPath.empty()
                ? QStringLiteral("EditorScene.nscene")
                : QDir(m_project.m_rootPath).filePath(QString::fromStdWString(currentPath.wstring())),
            QStringLiteral("Nexus Scene Files (*.nscene);;All Files (*.*)"));

        if (filePath.isEmpty())
        {
            return false;
        }

        NexusEngine::IO::AssetReference reference;
        if (assetReferenceRegistry)
        {
            reference = assetReferenceRegistry->GetOrCreateAssetReferenceByPath(std::filesystem::path(filePath.toStdWString()));
        }

        if (reference.IsEmpty())
        {
            statusBar()->showMessage(QStringLiteral("Failed to create scene asset reference"), 3000);
            return false;
        }

        m_sceneFileReference = reference;

        if (SaveActiveScene(filePath, QString::fromStdString(m_sceneFileReference.GetGuid())))
        {
            m_lastSavedSceneSnapshot = CaptureActiveSceneSnapshot();
            statusBar()->showMessage(QStringLiteral("Saved scene to %1").arg(filePath), 3000);
            return true;
        }

        statusBar()->showMessage(QStringLiteral("Failed to save scene"), 3000);
        return false;
    }
} // namespace NexusEditor
