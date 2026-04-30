#include "SceneViewWidget.h"

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

    bool SceneViewWidget::IsInitialized() const
    {
        return m_isInitialized;
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

        std::unique_ptr<NexusEngine::IGameApp> game = SampleGame::CreateGame();
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
