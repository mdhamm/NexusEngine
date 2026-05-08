#pragma once

#include <QString>
#include <QWidget>

#ifdef emit
#undef emit
#endif

#include <NexusEngine.h>

#include <chrono>
#include <functional>

namespace NexusEditor
{
    class EditorWindow;

    class SceneViewWidget final : public QWidget
    {
    public:
        /// <summary>
        /// Creates the editor scene view and its frame timer.
        /// </summary>
        /// <param name="editorWindow">Editor window that owns the engine instance.</param>
        /// <param name="parent">Optional parent widget.</param>
        explicit SceneViewWidget(EditorWindow& editorWindow, QWidget* parent = nullptr);

        /// <summary>
        /// Returns the engine instance hosted by the scene view.
        /// </summary>
        /// <returns>The hosted engine instance.</returns>
        NexusEngine::Engine* GetEngine();

        /// <summary>
        /// Returns the active scene hosted by the scene view.
        /// </summary>
        /// <returns>The active scene, or null if none is active.</returns>
        NexusEngine::Scene* GetActiveScene();

        /// <summary>
        /// Returns whether the embedded engine session has been initialized.
        /// </summary>
        /// <returns>True when the engine is ready to render; otherwise false.</returns>
        bool IsInitialized() const;

        /// <summary>
        /// Saves the active editor scene to a file.
        /// </summary>
        /// <param name="filePath">Destination file path.</param>
        /// <param name="assetGuid">Stable guid stored with the scene asset.</param>
        /// <returns>True if the scene was saved; otherwise false.</returns>
        bool SaveActiveScene(const QString& filePath, const QString& assetGuid) const;

        /// <summary>
        /// Loads the active editor scene from a file.
        /// </summary>
        /// <param name="filePath">Source scene file path.</param>
        /// <returns>True if the scene was loaded; otherwise false.</returns>
        bool LoadScene(const QString& filePath);

        /// <summary>
        /// Sets a callback invoked after the editor scene becomes available.
        /// </summary>
        /// <param name="callback">Callback to run when the scene is ready.</param>
        void SetSceneReadyCallback(std::function<void()> callback);

    protected:
        QPaintEngine* paintEngine() const override;
        void showEvent(QShowEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;

        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
        void leaveEvent(QEvent* event) override;

    private:
        void TickFrame();
        float ComputeDeltaSeconds();

        EditorWindow* m_editorWindow = nullptr;
        std::chrono::steady_clock::time_point m_previousFrameTime{};
        std::function<void()> m_onSceneReady;
        bool m_hasEmittedSceneReady = false;
    };
} // namespace NexusEditor
