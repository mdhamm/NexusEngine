#pragma once

#ifdef emit
#undef emit
#endif

#include <NexusEngine.h>
#include <filesystem/AssetReference.h>
#include <QDateTime>
#include <QMainWindow>

struct HINSTANCE__;
class QDialog;
class QDockWidget;
class QLabel;
class QPlainTextEdit;
class QProcess;

#include <string>
#include <functional>
#include <vector>

namespace NexusEngine
{
    struct Material;
}

#include "EditorProject.h"
#include "InspectedTarget.h"
#include "QtInputBackend.h"

namespace NexusEditor
{
    class ContentDrawerWidget;
    class PropertyWidget;
    class SceneGraphWidget;
    class SceneViewWidget;

    class EditorWindow final : public QMainWindow
    {
    public:
        /// <summary>
        /// Creates the main Nexus editor window.
        /// </summary>
        /// <param name="project">Project opened by the editor.</param>
        /// <param name="parent">Optional parent widget.</param>
        explicit EditorWindow(const EditorProject& project, QWidget* parent = nullptr);

        /// <summary>
        /// Destroys the main editor window.
        /// </summary>
        ~EditorWindow() override;

        /// <summary>
        /// Returns the engine instance hosted by the editor window.
        /// </summary>
        /// <returns>The hosted engine instance.</returns>
        NexusEngine::Engine* GetEngine();

        /// <summary>
        /// Returns the active scene hosted by the editor window.
        /// </summary>
        /// <returns>The active scene, or null if none is active.</returns>
        NexusEngine::Scene* GetActiveScene();

        /// <summary>
        /// Saves the active scene using the editor-owned engine.
        /// </summary>
        /// <param name="filePath">Destination scene path.</param>
        /// <param name="assetGuid">Stable scene asset guid.</param>
        /// <returns>True if the scene was saved; otherwise false.</returns>
        bool SaveActiveScene(const QString& filePath, const QString& assetGuid) const;

        /// <summary>
        /// Loads a scene into the editor-owned engine.
        /// </summary>
        /// <param name="sceneFilePath">Scene file path to load.</param>
        /// <returns>True if the scene was loaded; otherwise false.</returns>
        bool LoadScene(const std::filesystem::path& sceneFilePath);

        /// <summary>
        /// Advances the editor-owned engine by one frame.
        /// </summary>
        /// <param name="deltaSeconds">Frame delta time in seconds.</param>
        void TickEngineFrame(float deltaSeconds);

        /// <summary>
        /// Returns the editor input backend.
        /// </summary>
        /// <returns>The input backend used by the editor engine.</returns>
        QtInputBackend& GetInputBackend();

        /// <summary>
        /// Initializes the editor-owned engine against the scene view native window and a default game instance.
        /// </summary>
        void InitializeEngine();

        /// <summary>
        /// Resizes the editor-owned render target to match the scene view.
        /// </summary>
        /// <param name="width">New viewport width in pixels.</param>
        /// <param name="height">New viewport height in pixels.</param>
        void ResizeSceneViewport(int width, int height);

        /// <summary>
        /// Returns the absolute project root path used by the editor.
        /// </summary>
        /// <returns>The absolute project root path.</returns>
        const QString& GetProjectRootPath() const { return m_project.m_rootPath; }

        /// <summary>
        /// Returns the currently inspected editor target.
        /// </summary>
        /// <returns>The current inspected target.</returns>
        const InspectedTarget& GetInspectedTarget() const { return m_inspectedTarget; }

        /// <summary>
        /// Registers a listener that is invoked whenever the inspected target changes.
        /// </summary>
        /// <param name="listener">Callback receiving the new inspected target.</param>
        void AddInspectedTargetChangedListener(std::function<void(const InspectedTarget&)> listener);

    private:
        void BuildMenus();
        void BuildLayout();
        void ShowBuildOutput();
        void AppendBuildOutput(const QString& text);
        void ShowLoadingStatus(const QString& text);
        void HideLoadingStatus();
        void ResolveMaterialAssets();
        void SetSceneMode(bool isSceneMode);
        void ConfigureEditorCamera();
        void SetInspectedTarget(const InspectedTarget& inspectedTarget);
        bool BuildProjectGame();
        bool BuildAndLoadProjectGame();
        std::filesystem::path GetProjectGameBuildDirectory() const;
        std::filesystem::path GetProjectGameDllPath() const;
        bool HotReloadGame();
        bool LoadHotReloadedGame(const std::filesystem::path& sourceDllPath);
        void UnloadHotReloadedGame();
        QString ResolveSceneFilePath() const;
        std::string CaptureActiveSceneSnapshot() const;
        bool IsCurrentSceneDirty() const;
        bool SaveScene();
        bool SaveSceneAs();

        SceneViewWidget* m_sceneView = nullptr;
        SceneGraphWidget* m_sceneGraph = nullptr;
        PropertyWidget* m_propertyWidget = nullptr;
        ContentDrawerWidget* m_contentDrawer = nullptr;
        EditorProject m_project;
        NexusEngine::IO::AssetReference m_sceneFileReference;
        NexusEngine::Engine m_engine;
        QtInputBackend m_inputBackend;
        bool m_isSceneMode = true;
        bool m_hasLoadedScene = false;
        std::string m_lastSavedSceneSnapshot;
        InspectedTarget m_inspectedTarget;
        std::vector<std::function<void(const InspectedTarget&)>> m_inspectedTargetChangedListeners;

        struct MaterialAssetCacheEntry
        {
            QDateTime m_lastModified;
            std::shared_ptr<NexusEngine::Material> m_material;
        };

        QHash<QString, MaterialAssetCacheEntry> m_materialAssetCache;
        QDockWidget* m_buildOutputDock = nullptr;
        QPlainTextEdit* m_buildOutputTextEdit = nullptr;
        QDialog* m_loadingStatusDialog = nullptr;
        QLabel* m_loadingStatusLabel = nullptr;
        QPlainTextEdit* m_loadingStatusTextEdit = nullptr;
        QProcess* m_projectGameBuildProcess = nullptr;
        HINSTANCE__* m_hotReloadedGameModule = nullptr;
        NexusEngine::IGameApp* m_hotReloadedGame = nullptr;
        std::filesystem::path m_hotReloadedGameCopyPath;
        std::filesystem::path m_hotReloadedGamePdbCopyPath;
        using LoadGameFn = NexusEngine::IGameApp* (*)();
        using UnloadGameFn = void (*)(NexusEngine::IGameApp* game);
        UnloadGameFn m_hotReloadedGameUnloadFn = nullptr;
    };
} // namespace NexusEditor
