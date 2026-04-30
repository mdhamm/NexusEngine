#pragma once

#include <QWidget>

#ifdef emit
#undef emit
#endif

#include <NexusEngine.h>

#include <chrono>

namespace NexusEditor
{
    class SceneViewWidget final : public QWidget
    {
    public:
        /// <summary>
        /// Creates the editor scene view and its frame timer.
        /// </summary>
        /// <param name="parent">Optional parent widget.</param>
        explicit SceneViewWidget(QWidget* parent = nullptr);

        /// <summary>
        /// Shuts down the embedded engine session.
        /// </summary>
        ~SceneViewWidget() override;

        /// <summary>
        /// Returns the engine instance hosted by the scene view.
        /// </summary>
        /// <returns>The hosted engine instance.</returns>
        NexusEngine::Engine* GetEngine();

        /// <summary>
        /// Returns whether the embedded engine session has been initialized.
        /// </summary>
        /// <returns>True when the engine is ready to render; otherwise false.</returns>
        bool IsInitialized() const;

    protected:
        QPaintEngine* paintEngine() const override;
        void showEvent(QShowEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;

    private:
        void EnsureEngineInitialized();
        void TickFrame();
        float ComputeDeltaSeconds();

        NexusEngine::Engine m_engine;
        bool m_isInitialized = false;
        std::chrono::steady_clock::time_point m_previousFrameTime{};
    };
} // namespace NexusEditor
