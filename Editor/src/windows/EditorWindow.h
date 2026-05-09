#pragma once

#ifdef emit
#undef emit
#endif

#include <NexusEngine.h>
#include <filesystem/AssetReference.h>
#include <QDateTime>
#include <QMainWindow>

namespace NexusEngine
{
    struct Material;
}

#include "EditorProject.h"
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
        ~EditorWindow() override = default;

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

    private:
        void BuildMenus();
        void BuildLayout();
        void ResolveMaterialAssets();
        void SetSceneMode(bool isSceneMode);
        void ConfigureEditorCamera();
        bool ResolveSceneFilePath();
        void SaveScene();
        void SaveSceneAs();

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

        struct MaterialAssetCacheEntry
        {
            QDateTime m_lastModified;
            std::shared_ptr<NexusEngine::Material> m_material;
        };

        QHash<QString, MaterialAssetCacheEntry> m_materialAssetCache;
    };
} // namespace NexusEditor
