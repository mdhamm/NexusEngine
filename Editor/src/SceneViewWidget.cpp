#include "SceneViewWidget.h"

#include "EditorSceneApp.h"
#include "EditorSceneSerializer.h"

#include <QResizeEvent>
#include <QTimer>

#include <Game.h>

#include <SDL.h>

#include <algorithm>

namespace NexusEditor
{
    namespace
    {
        constexpr int DefaultFrameIntervalMs = 16;
        constexpr float DefaultDeltaSeconds = 1.0f / 60.0f;
        constexpr float MaxDeltaSeconds = 0.1f;
    }

    SceneViewWidget::SceneViewWidget(QWidget* parent)
        : QWidget(parent)
    {
        setWindowTitle(QStringLiteral("Scene"));
        setAttribute(Qt::WA_NativeWindow, true);
        setAttribute(Qt::WA_PaintOnScreen, true);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAutoFillBackground(false);
        setMinimumSize(640, 360);
        setFocusPolicy(Qt::StrongFocus);

        auto* frameTimer = new QTimer(this);
        frameTimer->setInterval(DefaultFrameIntervalMs);
        connect(frameTimer, &QTimer::timeout, this, [this]() { TickFrame(); });
        frameTimer->start();
    }

    SceneViewWidget::~SceneViewWidget()
    {
        if (m_isInitialized)
        {
            m_engine.Shutdown();
        }
    }

    NexusEngine::Engine* SceneViewWidget::GetEngine()
    {
        return &m_engine;
    }

    NexusEngine::Scene* SceneViewWidget::GetActiveScene()
    {
        return m_engine.ActiveScene();
    }

    bool SceneViewWidget::IsInitialized() const
    {
        return m_isInitialized;
    }

    bool SceneViewWidget::SaveActiveScene(const QString& filePath, const QString& assetGuid) const
    {
        if (!m_isInitialized || filePath.isEmpty())
        {
            return false;
        }

        NexusEngine::Scene* activeScene = const_cast<SceneViewWidget*>(this)->m_engine.ActiveScene();
        return activeScene ? SaveSceneToFile(*activeScene, filePath, assetGuid) : false;
    }

    bool SceneViewWidget::LoadScene(const QString& filePath)
    {
        EnsureEngineInitialized();
        if (!m_isInitialized || filePath.isEmpty())
        {
            return false;
        }

        NexusEngine::Scene* activeScene = m_engine.ActiveScene();
        if (!activeScene || !LoadSceneFromFile(*activeScene, filePath))
        {
            return false;
        }

        if (m_onSceneReady)
        {
            m_onSceneReady();
        }

        return true;
    }

    void SceneViewWidget::SetSceneReadyCallback(std::function<void()> callback)
    {
        m_onSceneReady = std::move(callback);
    }

    QPaintEngine* SceneViewWidget::paintEngine() const
    {
        return nullptr;
    }

    void SceneViewWidget::showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);
        EnsureEngineInitialized();
    }

    void SceneViewWidget::resizeEvent(QResizeEvent* event)
    {
        QWidget::resizeEvent(event);

        if (!m_isInitialized)
        {
            EnsureEngineInitialized();
        }
    }

    void SceneViewWidget::EnsureEngineInitialized()
    {
        if (m_isInitialized)
        {
            return;
        }

        createWinId();

        SampleGame::RegisterEditorComponentDescriptors();

        std::unique_ptr<NexusEngine::IGameApp> game = std::make_unique<EditorSceneApp>();
        if (!game)
        {
            return;
        }

        NexusEngine::NativeWindow nativeWindow{};
        nativeWindow.m_width = std::max(1, width());
        nativeWindow.m_height = std::max(1, height());
        nativeWindow.m_hWnd = reinterpret_cast<void*>(winId());

        m_isInitialized = m_engine.Initialize(nativeWindow, std::move(game));
        if (m_isInitialized)
        {
            m_previousFrameTime = std::chrono::steady_clock::now();
            TickFrame();

            if (m_onSceneReady)
            {
                m_onSceneReady();
            }
        }
    }

    void SceneViewWidget::TickFrame()
    {
        EnsureEngineInitialized();
        if (!m_isInitialized)
        {
            return;
        }

        m_engine.RunFrame(ComputeDeltaSeconds());
    }

    float SceneViewWidget::ComputeDeltaSeconds()
    {
        const auto now = std::chrono::steady_clock::now();
        if (m_previousFrameTime == std::chrono::steady_clock::time_point{})
        {
            m_previousFrameTime = now;
            return DefaultDeltaSeconds;
        }

        const std::chrono::duration<float> delta = now - m_previousFrameTime;
        m_previousFrameTime = now;
        return std::clamp(delta.count(), 0.0f, MaxDeltaSeconds);
    }
} // namespace NexusEditor
