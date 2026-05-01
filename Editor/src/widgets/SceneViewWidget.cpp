#include "SceneViewWidget.h"

#include "EditorWindow.h"
#include "EditorSceneSerializer.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QShowEvent>
#include <QTimer>
#include <QWheelEvent>

#include <algorithm>

namespace NexusEditor
{
    namespace
    {
        constexpr int DefaultFrameIntervalMs = 16;
        constexpr float DefaultDeltaSeconds = 1.0f / 60.0f;
        constexpr float MaxDeltaSeconds = 0.1f;
    }

    SceneViewWidget::SceneViewWidget(EditorWindow& editorWindow, QWidget* parent)
        : QWidget(parent)
        , m_editorWindow(&editorWindow)
    {
        setWindowTitle(QStringLiteral("Scene"));
        setAttribute(Qt::WA_NativeWindow, true);
        setAttribute(Qt::WA_PaintOnScreen, true);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAutoFillBackground(false);
        setMinimumSize(640, 360);
        setFocusPolicy(Qt::StrongFocus);
        setMouseTracking(true);

        auto* frameTimer = new QTimer(this);
        frameTimer->setInterval(DefaultFrameIntervalMs);
        connect(frameTimer, &QTimer::timeout, this, [this]() { TickFrame(); });
        frameTimer->start();
    }

    NexusEngine::Engine* SceneViewWidget::GetEngine()
    {
        return m_editorWindow ? m_editorWindow->GetEngine() : nullptr;
    }

    NexusEngine::Scene* SceneViewWidget::GetActiveScene()
    {
        return m_editorWindow ? m_editorWindow->GetActiveScene() : nullptr;
    }

    bool SceneViewWidget::IsInitialized() const
    {
        return m_editorWindow && m_editorWindow->IsEngineInitialized();
    }

    bool SceneViewWidget::SaveActiveScene(const QString& filePath, const QString& assetGuid) const
    {
        return m_editorWindow && m_editorWindow->SaveActiveScene(filePath, assetGuid);
    }

    bool SceneViewWidget::LoadScene(const QString& filePath)
    {
        if (!m_editorWindow || !m_editorWindow->LoadScene(filePath))
        {
            return false;
        }

        m_hasEmittedSceneReady = true;
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
        if (m_editorWindow)
        {
            m_editorWindow->EnsureEngineInitialized();
        }
    }

    void SceneViewWidget::resizeEvent(QResizeEvent* event)
    {
        QWidget::resizeEvent(event);

        if (m_editorWindow && !m_editorWindow->IsEngineInitialized())
        {
            m_editorWindow->EnsureEngineInitialized();
        }
    }

    void SceneViewWidget::keyPressEvent(QKeyEvent* event)
    {
        if (m_editorWindow)
        {
            m_editorWindow->GetInputBackend().OnKeyPress(event);
        }
        event->accept();
    }

    void SceneViewWidget::keyReleaseEvent(QKeyEvent* event)
    {
        if (m_editorWindow)
        {
            m_editorWindow->GetInputBackend().OnKeyRelease(event);
        }
        event->accept();
    }

    void SceneViewWidget::mouseMoveEvent(QMouseEvent* event)
    {
        if (m_editorWindow)
        {
            m_editorWindow->GetInputBackend().OnMouseMove(event);
        }
        event->accept();
    }

    void SceneViewWidget::mousePressEvent(QMouseEvent* event)
    {
        setFocus();
        if (m_editorWindow)
        {
            m_editorWindow->GetInputBackend().OnMousePress(event);
        }
        event->accept();
    }

    void SceneViewWidget::mouseReleaseEvent(QMouseEvent* event)
    {
        if (m_editorWindow)
        {
            m_editorWindow->GetInputBackend().OnMouseRelease(event);
        }
        event->accept();
    }

    void SceneViewWidget::wheelEvent(QWheelEvent* event)
    {
        if (m_editorWindow)
        {
            m_editorWindow->GetInputBackend().OnWheel(event);
        }
        event->accept();
    }

    void SceneViewWidget::leaveEvent(QEvent* event)
    {
        if (m_editorWindow)
        {
            m_editorWindow->GetInputBackend().ResetMouseTracking();
        }
        QWidget::leaveEvent(event);
    }

    void SceneViewWidget::TickFrame()
    {
        if (!m_editorWindow)
        {
            return;
        }

        m_editorWindow->EnsureEngineInitialized();
        if (!m_editorWindow->IsEngineInitialized())
        {
            return;
        }

        m_editorWindow->TickEngineFrame(ComputeDeltaSeconds());

        if (!m_hasEmittedSceneReady && GetActiveScene())
        {
            m_hasEmittedSceneReady = true;
            if (m_onSceneReady)
            {
                m_onSceneReady();
            }
        }
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
