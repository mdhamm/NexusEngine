#include "EditorWindow.h"

#include "EditorProjectRegistry.h"
#include "EditorSceneSerializer.h"
#include "widgets/ContentDrawerWidget.h"
#include "widgets/PropertyWidget.h"
#include "widgets/SceneGraphWidget.h"
#include "widgets/SceneViewWidget.h"

#include <assets/MaterialAsset.h>
#include <EngineComponentRegistration.h>
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
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStatusBar>
#include <QVBoxLayout>

#include <Windows.h>

#ifdef emit
#undef emit
#endif

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <system_error>

#ifndef NEXUS_EDITOR_CONTENT_ROOT
#define NEXUS_EDITOR_CONTENT_ROOT "."
#endif

#ifdef max
#undef max
#endif

namespace NexusEditor
{
    namespace
    {
        QString GetWorkspaceCMakeCacheValue(const QString& key)
        {
            const QString cachePath = QDir(QStringLiteral(NEXUS_EDITOR_CONTENT_ROOT)).filePath(QStringLiteral("Intermediate/x64-debug/CMakeCache.txt"));
            QFile cacheFile(cachePath);
            if (!cacheFile.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                return {};
            }

            const QByteArray keyPrefix = key.toUtf8() + ':';
            while (!cacheFile.atEnd())
            {
                const QByteArray line = cacheFile.readLine().trimmed();
                if (!line.startsWith(keyPrefix))
                {
                    continue;
                }

                const int equalsIndex = line.indexOf('=');
                if (equalsIndex < 0)
                {
                    return {};
                }

                return QString::fromUtf8(line.mid(equalsIndex + 1));
            }

            return {};
        }

        QString QuoteForCmd(const QString& value)
        {
            QString escapedValue = value;
            escapedValue.replace('"', QStringLiteral(""""));
            return QStringLiteral("\"%1\"").arg(escapedValue);
        }

        QString QuoteCommandArgument(const QString& value)
        {
            QString escapedValue = value;
            escapedValue.replace('"', QStringLiteral("\\\""));
            return QStringLiteral("\"%1\"").arg(escapedValue);
        }

        QString BuildCommandString(const QString& executable, const QStringList& arguments)
        {
            QStringList commandParts;
            commandParts.push_back(QuoteCommandArgument(executable));
            for (const QString& argument : arguments)
            {
                commandParts.push_back(QuoteCommandArgument(argument));
            }

            return commandParts.join(' ');
        }

        void StartCmdProcess(QProcess& process, const QString& command)
        {
            process.setProgram(QStringLiteral("cmd.exe"));
            process.setNativeArguments(QStringLiteral("/c ") + command);
            process.start();
        }

        QString TryGetVcVars64Path(const QString& toolPath)
        {
            if (toolPath.isEmpty())
            {
                return {};
            }

            std::filesystem::path currentPath = std::filesystem::path(toolPath.toStdWString()).parent_path();
            while (!currentPath.empty())
            {
                if (currentPath.filename() == L"VC")
                {
                    const std::filesystem::path vcvarsPath = currentPath / L"Auxiliary" / L"Build" / L"vcvars64.bat";
                    if (std::filesystem::exists(vcvarsPath))
                    {
                        return QString::fromStdWString(vcvarsPath.wstring());
                    }

                    break;
                }

                currentPath = currentPath.parent_path();
            }

            return {};
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

    EditorWindow::~EditorWindow()
    {
        if (m_projectGameBuildProcess)
        {
            m_projectGameBuildProcess->kill();
            m_projectGameBuildProcess->deleteLater();
            m_projectGameBuildProcess = nullptr;
        }

        UnloadHotReloadedGame();
        if (m_engine.IsInitialized())
        {
            m_engine.Shutdown();
        }
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
        auto* buildOutputAction = fileMenu->addAction(QStringLiteral("&Build Output"));
        connect(buildOutputAction, &QAction::triggered, this, [this]() { ShowBuildOutput(); });
        fileMenu->addSeparator();
        auto* hotReloadGameAction = fileMenu->addAction(QStringLiteral("&Hot Reload Game"));
        connect(hotReloadGameAction, &QAction::triggered, this, [this]() { HotReloadGame(); });
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

        m_buildOutputDock = new QDockWidget(QStringLiteral("Build Output"), this);
        m_buildOutputDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
        m_buildOutputTextEdit = new QPlainTextEdit(m_buildOutputDock);
        m_buildOutputTextEdit->setReadOnly(true);
        m_buildOutputDock->setWidget(m_buildOutputTextEdit);
        addDockWidget(Qt::BottomDockWidgetArea, m_buildOutputDock);
        m_buildOutputDock->hide();

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

        NexusEngine::NativeWindow nativeWindow{};
        nativeWindow.m_width = std::max(1, m_sceneView->width());
        nativeWindow.m_height = std::max(1, m_sceneView->height());
        nativeWindow.m_hWnd = reinterpret_cast<void*>(m_sceneView->winId());
        m_engine.Initialize(nativeWindow, m_project.m_rootPath.toStdString());
        NexusEngine::RegisterEngineComponentTypes(m_engine.GetWorld());

        if (m_project.m_requiresInitialBuild)
        {
            if (BuildProjectGame())
            {
                m_project.m_requiresInitialBuild = false;
                EditorProjectRegistry::AddRecentProject(m_project);
            }
        }

        (void)LoadProjectGameFromConfiguredOutput();

        NexusEngine::Scene& sceneRef = m_engine.CreateScene("EditorScene");
        m_engine.SetActiveScene(sceneRef);
    }

    void EditorWindow::ShowBuildOutput()
    {
        if (!m_buildOutputDock)
        {
            return;
        }

        m_buildOutputDock->show();
        m_buildOutputDock->raise();
    }

    void EditorWindow::AppendBuildOutput(const QString& text)
    {
        if (text.isEmpty())
        {
            return;
        }

        const auto appendToTextEdit = [&](QPlainTextEdit* textEdit)
        {
            if (!textEdit)
            {
                return;
            }

            textEdit->appendPlainText(text);
            QTextCursor cursor = textEdit->textCursor();
            cursor.movePosition(QTextCursor::End);
            textEdit->setTextCursor(cursor);
        };

        appendToTextEdit(m_buildOutputTextEdit);
        appendToTextEdit(m_loadingStatusTextEdit);
    }

    void EditorWindow::ShowLoadingStatus(const QString& text)
    {
        if (!m_loadingStatusDialog)
        {
            m_loadingStatusDialog = new QDialog(this, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
            m_loadingStatusDialog->setWindowTitle(QStringLiteral("Loading Project"));
            m_loadingStatusDialog->setModal(true);
            m_loadingStatusDialog->resize(700, 420);

            auto* layout = new QVBoxLayout(m_loadingStatusDialog);
            m_loadingStatusLabel = new QLabel(m_loadingStatusDialog);
            layout->addWidget(m_loadingStatusLabel);

            m_loadingStatusTextEdit = new QPlainTextEdit(m_loadingStatusDialog);
            m_loadingStatusTextEdit->setReadOnly(true);
            layout->addWidget(m_loadingStatusTextEdit, 1);
        }

        if (m_loadingStatusLabel)
        {
            m_loadingStatusLabel->setText(text);
        }

        if (m_loadingStatusTextEdit && !m_loadingStatusDialog->isVisible())
        {
            m_loadingStatusTextEdit->clear();
        }

        m_loadingStatusDialog->show();
        m_loadingStatusDialog->raise();
    }

    void EditorWindow::HideLoadingStatus()
    {
        if (m_loadingStatusDialog)
        {
            m_loadingStatusDialog->hide();
        }
    }

    bool EditorWindow::BuildProjectGame()
    {
        if (m_projectGameBuildProcess)
        {
            AppendBuildOutput(QStringLiteral("A project game build is already in progress."));
            ShowBuildOutput();
            return false;
        }

        const std::filesystem::path projectRoot = std::filesystem::path(m_project.m_rootPath.toStdWString());
        const QString editorBuildConfigurationName = GetEditorBuildConfigurationName();
        const std::filesystem::path buildDirectory = std::filesystem::path(m_project.m_rootPath.toStdWString()) / L"Intermediate" / std::filesystem::path(editorBuildConfigurationName.toStdWString());

        if (m_buildOutputTextEdit)
        {
            m_buildOutputTextEdit->clear();
        }

        AppendBuildOutput(QStringLiteral("Configuring project game build..."));
        AppendBuildOutput(QStringLiteral("Working directory: %1").arg(QString::fromStdWString(projectRoot.wstring())));
        AppendBuildOutput(QStringLiteral("Build directory: %1").arg(QString::fromStdWString(buildDirectory.wstring())));
        ShowLoadingStatus(QStringLiteral("Configuring project game build..."));

        const QString rcCompilerPath = GetWorkspaceCMakeCacheValue(QStringLiteral("CMAKE_RC_COMPILER"));
        const QString mtPath = GetWorkspaceCMakeCacheValue(QStringLiteral("CMAKE_MT"));
        const QString makeProgramPath = GetWorkspaceCMakeCacheValue(QStringLiteral("CMAKE_MAKE_PROGRAM"));
        const QString linkerPath = GetWorkspaceCMakeCacheValue(QStringLiteral("CMAKE_LINKER"));
        const QString vcvarsPath = TryGetVcVars64Path(linkerPath);
        if (!rcCompilerPath.isEmpty())
        {
            AppendBuildOutput(QStringLiteral("Using RC compiler: %1").arg(rcCompilerPath));
        }
        if (!mtPath.isEmpty())
        {
            AppendBuildOutput(QStringLiteral("Using manifest tool: %1").arg(mtPath));
        }
        if (!vcvarsPath.isEmpty())
        {
            AppendBuildOutput(QStringLiteral("Using VC environment script: %1").arg(vcvarsPath));
        }

        QProcessEnvironment processEnvironment = QProcessEnvironment::systemEnvironment();
        QString pathValue = processEnvironment.value(QStringLiteral("PATH"));
        const auto prependToolDirectoryToPath = [&](const QString& toolPath)
        {
            if (toolPath.isEmpty())
            {
                return;
            }

            const QString toolDirectory = QFileInfo(toolPath).absolutePath();
            if (toolDirectory.isEmpty())
            {
                return;
            }

            AppendBuildOutput(QStringLiteral("Adding tool directory to PATH: %1").arg(toolDirectory));
            pathValue = toolDirectory + QDir::listSeparator() + pathValue;
        };

        prependToolDirectoryToPath(rcCompilerPath);
        prependToolDirectoryToPath(mtPath);
        prependToolDirectoryToPath(makeProgramPath);
        processEnvironment.insert(QStringLiteral("PATH"), pathValue);
        if (!rcCompilerPath.isEmpty())
        {
            processEnvironment.insert(QStringLiteral("RC"), rcCompilerPath);
        }

        std::error_code errorCode;
        std::filesystem::create_directories(buildDirectory, errorCode);
        if (errorCode)
        {
            AppendBuildOutput(QStringLiteral("Failed to create project game build directory: %1").arg(QString::fromStdString(errorCode.message())));
            ShowBuildOutput();
            HideLoadingStatus();
            statusBar()->showMessage(QStringLiteral("Failed to create project game build directory"), 3000);
            return false;
        }

        m_projectGameBuildProcess = new QProcess(this);
        m_projectGameBuildProcess->setWorkingDirectory(QString::fromStdWString(projectRoot.wstring()));
        m_projectGameBuildProcess->setProcessEnvironment(processEnvironment);
        QStringList configureArguments = {
            QStringLiteral("-S"),
            QStringLiteral("."),
            QStringLiteral("-B"),
            QString::fromStdWString(buildDirectory.wstring()),
            QStringLiteral("-G"),
            QStringLiteral("Ninja"),
            QStringLiteral("-DCMAKE_BUILD_TYPE:STRING=Debug"),
            QStringLiteral("-DNEXUS_PROJECT_OUTPUT_SUBDIR:STRING=%1").arg(editorBuildConfigurationName),
            QStringLiteral("-DCMAKE_RC_COMPILER:FILEPATH=%1").arg(rcCompilerPath),
            QStringLiteral("-DCMAKE_MT:FILEPATH=%1").arg(mtPath)
        };
        QString configureCommand = BuildCommandString(QStringLiteral("cmake"), configureArguments);
        if (!vcvarsPath.isEmpty())
        {
            configureCommand = QStringLiteral("call %1 >nul && %2").arg(QuoteForCmd(vcvarsPath), configureCommand);
        }
        StartCmdProcess(*m_projectGameBuildProcess, configureCommand);
        connect(m_projectGameBuildProcess, &QProcess::readyReadStandardOutput, this,
            [this]()
            {
                if (m_projectGameBuildProcess)
                {
                    AppendBuildOutput(QString::fromLocal8Bit(m_projectGameBuildProcess->readAllStandardOutput()));
                }
            });
        connect(m_projectGameBuildProcess, &QProcess::readyReadStandardError, this,
            [this]()
            {
                if (m_projectGameBuildProcess)
                {
                    AppendBuildOutput(QString::fromLocal8Bit(m_projectGameBuildProcess->readAllStandardError()));
                }
            });
        while (m_projectGameBuildProcess->state() != QProcess::NotRunning)
        {
            QCoreApplication::processEvents();
        }
        if (m_projectGameBuildProcess->exitStatus() != QProcess::NormalExit || m_projectGameBuildProcess->exitCode() != 0)
        {
            AppendBuildOutput(QStringLiteral("Configure step failed with exit code %1.").arg(m_projectGameBuildProcess->exitCode()));
            ShowBuildOutput();
            HideLoadingStatus();
            m_projectGameBuildProcess->deleteLater();
            m_projectGameBuildProcess = nullptr;
            statusBar()->showMessage(QStringLiteral("Failed to configure project game build"), 3000);
            return false;
        }

        AppendBuildOutput(QStringLiteral("Building project game DLL..."));
        ShowLoadingStatus(QStringLiteral("Building project game DLL..."));
        m_projectGameBuildProcess->setWorkingDirectory(QString::fromStdWString(projectRoot.wstring()));
        m_projectGameBuildProcess->setProcessEnvironment(processEnvironment);
        QStringList buildArguments = {
            QStringLiteral("--build"),
            QString::fromStdWString(buildDirectory.wstring()),
            QStringLiteral("--config"),
            QStringLiteral("Debug"),
            QStringLiteral("--target"),
            QStringLiteral("ProjectGame")
        };
        QString buildCommand = BuildCommandString(QStringLiteral("cmake"), buildArguments);
        if (!vcvarsPath.isEmpty())
        {
            buildCommand = QStringLiteral("call %1 >nul && %2").arg(QuoteForCmd(vcvarsPath), buildCommand);
        }
        StartCmdProcess(*m_projectGameBuildProcess, buildCommand);
        while (m_projectGameBuildProcess->state() != QProcess::NotRunning)
        {
            QCoreApplication::processEvents();
        }
        if (m_projectGameBuildProcess->exitStatus() != QProcess::NormalExit || m_projectGameBuildProcess->exitCode() != 0)
        {
            AppendBuildOutput(QStringLiteral("Build step failed with exit code %1.").arg(m_projectGameBuildProcess->exitCode()));
            ShowBuildOutput();
            HideLoadingStatus();
            m_projectGameBuildProcess->deleteLater();
            m_projectGameBuildProcess = nullptr;
            statusBar()->showMessage(QStringLiteral("Failed to build project game DLL"), 3000);
            return false;
        }

        HideLoadingStatus();
        m_projectGameBuildProcess->deleteLater();
        m_projectGameBuildProcess = nullptr;
        AppendBuildOutput(QStringLiteral("Project game build completed successfully."));
        return true;
    }

    bool EditorWindow::LoadProjectGameFromConfiguredOutput()
    {
        if (m_buildOutputTextEdit)
        {
            m_buildOutputTextEdit->clear();
        }

        AppendBuildOutput(QStringLiteral("Loading project game from configured output..."));
        ShowLoadingStatus(QStringLiteral("Loading project game from configured output..."));
        const std::filesystem::path projectGameDllPath = GetProjectGameDllPath();
        AppendBuildOutput(QStringLiteral("Configured DLL path: %1").arg(QString::fromStdWString(projectGameDllPath.wstring())));
        if (projectGameDllPath.empty())
        {
            AppendBuildOutput(QStringLiteral("Configured project game DLL was not found."));
            for (const std::filesystem::path& candidatePath : GetProjectGameDllCandidatePaths())
            {
                AppendBuildOutput(QStringLiteral("Checked DLL path: %1").arg(QString::fromStdWString(candidatePath.wstring())));
            }
            ShowBuildOutput();
            HideLoadingStatus();
            statusBar()->showMessage(QStringLiteral("Configured project game DLL was not found"), 3000);
            return false;
        }

        const bool loaded = LoadHotReloadedGame(projectGameDllPath);
        HideLoadingStatus();
        return loaded;
    }

    QString EditorWindow::GetEditorBuildConfigurationName() const
    {
#ifdef NDEBUG
        return QStringLiteral("editor-release");
#else
        return QStringLiteral("editor-debug");
#endif
    }

    std::vector<std::filesystem::path> EditorWindow::GetProjectGameDllCandidatePaths() const
    {
        const std::filesystem::path projectRoot = std::filesystem::path(m_project.m_rootPath.toStdWString());
        const std::wstring projectGameDllName = L"ProjectGame.dll";

        std::vector<std::filesystem::path> candidatePaths;
        const auto appendCandidate = [&](const std::wstring& subdirectoryName)
        {
            const std::filesystem::path candidatePath = projectRoot / L"Binaries" / subdirectoryName / projectGameDllName;
            if (std::find(candidatePaths.begin(), candidatePaths.end(), candidatePath) == candidatePaths.end())
            {
                candidatePaths.push_back(candidatePath);
            }
        };

        appendCandidate(GetEditorBuildConfigurationName().toStdWString());
#ifdef NDEBUG
        appendCandidate(L"x64-release");
        appendCandidate(L"web-release");
        appendCandidate(L"editor-release");
#else
        appendCandidate(L"x64-debug");
        appendCandidate(L"web-debug");
        appendCandidate(L"editor-debug");
#endif

        return candidatePaths;
    }

    std::filesystem::path EditorWindow::GetProjectGameDllPath() const
    {
        for (const std::filesystem::path& candidatePath : GetProjectGameDllCandidatePaths())
        {
            if (std::filesystem::exists(candidatePath))
            {
                return candidatePath;
            }
        }

        return {};
    }

    bool EditorWindow::HotReloadGame()
    {
        return LoadProjectGameFromConfiguredOutput();
    }

    bool EditorWindow::LoadHotReloadedGame(const std::filesystem::path& sourceDllPath)
    {
        if (!m_engine.IsInitialized())
        {
            return false;
        }

        UnloadHotReloadedGame();

        const std::filesystem::path sourceDirectory = sourceDllPath.parent_path();
        const std::filesystem::path hotReloadDirectory = std::filesystem::path(m_project.m_rootPath.toStdWString()) / L"Temp" / L"HotReload";
        std::error_code errorCode;
        std::filesystem::create_directories(hotReloadDirectory, errorCode);
        if (errorCode)
        {
            statusBar()->showMessage(QStringLiteral("Failed to prepare hot reload directory"), 3000);
            return false;
        }

        const QString uniqueSuffix = QString::number(QDateTime::currentMSecsSinceEpoch());
        m_hotReloadedGameCopyPath = hotReloadDirectory / std::filesystem::path((QStringLiteral("SampleGame_") + uniqueSuffix + QStringLiteral(".dll")).toStdWString());
        m_hotReloadedGamePdbCopyPath = hotReloadDirectory / std::filesystem::path((QStringLiteral("SampleGame_") + uniqueSuffix + QStringLiteral(".pdb")).toStdWString());

        std::filesystem::copy_file(sourceDllPath, m_hotReloadedGameCopyPath, std::filesystem::copy_options::overwrite_existing, errorCode);
        if (errorCode)
        {
            statusBar()->showMessage(QStringLiteral("Failed to copy game DLL for hot reload"), 3000);
            return false;
        }

        std::filesystem::path pdbPath = sourceDllPath;
        pdbPath.replace_extension(L".pdb");
        if (std::filesystem::exists(pdbPath))
        {
            std::filesystem::copy_file(pdbPath, m_hotReloadedGamePdbCopyPath, std::filesystem::copy_options::overwrite_existing, errorCode);
            errorCode.clear();
        }

        m_hotReloadedGameModule = ::LoadLibraryW(m_hotReloadedGameCopyPath.c_str());
        if (!m_hotReloadedGameModule)
        {
            statusBar()->showMessage(QStringLiteral("Failed to load game DLL"), 3000);
            return false;
        }

        auto loadGameFn = reinterpret_cast<LoadGameFn>(::GetProcAddress(reinterpret_cast<HMODULE>(m_hotReloadedGameModule), "LoadGame"));
        m_hotReloadedGameUnloadFn = reinterpret_cast<UnloadGameFn>(::GetProcAddress(reinterpret_cast<HMODULE>(m_hotReloadedGameModule), "UnloadGame"));
        if (!loadGameFn || !m_hotReloadedGameUnloadFn)
        {
            UnloadHotReloadedGame();
            statusBar()->showMessage(QStringLiteral("Game DLL is missing LoadGame or UnloadGame exports"), 3000);
            return false;
        }

        m_hotReloadedGame = loadGameFn();
        if (!m_hotReloadedGame)
        {
            UnloadHotReloadedGame();
            statusBar()->showMessage(QStringLiteral("Game DLL failed to create a game instance"), 3000);
            return false;
        }

        m_engine.LoadGame(*m_hotReloadedGame);
        statusBar()->showMessage(QStringLiteral("Hot reloaded game from %1").arg(QString::fromStdWString(sourceDllPath.wstring())), 3000);
        return true;
    }

    void EditorWindow::UnloadHotReloadedGame()
    {
        if (m_hotReloadedGame)
        {
            m_engine.UnloadGame();
            if (m_hotReloadedGameUnloadFn)
            {
                m_hotReloadedGameUnloadFn(m_hotReloadedGame);
            }
            m_hotReloadedGame = nullptr;
        }

        m_hotReloadedGameUnloadFn = nullptr;

        if (m_hotReloadedGameModule)
        {
            ::FreeLibrary(reinterpret_cast<HMODULE>(m_hotReloadedGameModule));
            m_hotReloadedGameModule = nullptr;
        }

        if (!m_hotReloadedGameCopyPath.empty())
        {
            std::error_code errorCode;
            std::filesystem::remove(m_hotReloadedGameCopyPath, errorCode);
            m_hotReloadedGameCopyPath.clear();
        }

        if (!m_hotReloadedGamePdbCopyPath.empty())
        {
            std::error_code errorCode;
            std::filesystem::remove(m_hotReloadedGamePdbCopyPath, errorCode);
            m_hotReloadedGamePdbCopyPath.clear();
        }
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
