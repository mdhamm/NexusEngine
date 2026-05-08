#pragma once

#include "input/InputState.h"

#include <QKeyEvent>
#include <optional>

namespace NexusEditor
{
    class QtInputBackend final : public NexusEngine::IInputBackend
    {
    public:
        void BeginFrame() override
        {
            // Do not clear keys/buttons here.
            // Qt key/button state changes are event-driven.
        }

        void EndFrame() override
        {
            if (m_inputState)
            {
                m_inputState->ClearFrameDeltas();
            }
        }

        void OnKeyPress(QKeyEvent* event)
        {
            if (!event || event->isAutoRepeat())
            {
                return;
            }

            const std::optional<NexusEngine::InputKey> key = TranslateQtKey(event->key(), event->nativeScanCode());
            if (key.has_value())
            {
                if (m_inputState)
                {
                    m_inputState->keys[static_cast<size_t>(*key)] = true;
                }
            }
        }

        void OnKeyRelease(QKeyEvent* event)
        {
            if (!event || event->isAutoRepeat())
            {
                return;
            }

            const std::optional<NexusEngine::InputKey> key = TranslateQtKey(event->key(), event->nativeScanCode());
            if (key.has_value())
            {
                if (m_inputState)
                {
                    m_inputState->keys[static_cast<size_t>(*key)] = false;
                }
            }
        }

        void OnMouseMove(QMouseEvent* event)
        {
            if (!event)
            {
                return;
            }

            if (!m_inputState)
            {
                return;
            }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            const QPoint position = event->position().toPoint();
#else
            const QPoint position = event->pos();
#endif

            m_inputState->mouseX = position.x();
            m_inputState->mouseY = position.y();

            if (m_hasLastMousePosition)
            {
                const QPoint delta = position - m_lastMousePosition;
                m_inputState->mouseDeltaX += delta.x();
                m_inputState->mouseDeltaY += delta.y();
            }

            m_lastMousePosition = position;
            m_hasLastMousePosition = true;
        }

        void OnMousePress(QMouseEvent* event)
        {
            if (!event)
            {
                return;
            }

            SetMouseButton(event->button(), true);
        }

        void OnMouseRelease(QMouseEvent* event)
        {
            if (!event)
            {
                return;
            }

            SetMouseButton(event->button(), false);
        }

        void OnWheel(QWheelEvent* event)
        {
            if (!event)
            {
                return;
            }

            if (!m_inputState)
            {
                return;
            }

            m_inputState->mouseWheelDelta += static_cast<float>(event->angleDelta().y()) / 120.0f;
        }

        void ResetMouseTracking()
        {
            m_hasLastMousePosition = false;
            m_lastMousePosition = {};
        }

    private:
        QPoint m_lastMousePosition{};
        bool m_hasLastMousePosition = false;

        static std::optional<NexusEngine::InputKey> TranslateQtKey(int qtKey, quint32 nativeScanCode)
        {
            using NexusEngine::InputKey;

            switch (qtKey)
            {
            case Qt::Key_A: return InputKey::A;
            case Qt::Key_B: return InputKey::B;
            case Qt::Key_C: return InputKey::C;
            case Qt::Key_D: return InputKey::D;
            case Qt::Key_E: return InputKey::E;
            case Qt::Key_F: return InputKey::F;
            case Qt::Key_G: return InputKey::G;
            case Qt::Key_H: return InputKey::H;
            case Qt::Key_I: return InputKey::I;
            case Qt::Key_J: return InputKey::J;
            case Qt::Key_K: return InputKey::K;
            case Qt::Key_L: return InputKey::L;
            case Qt::Key_M: return InputKey::M;
            case Qt::Key_N: return InputKey::N;
            case Qt::Key_O: return InputKey::O;
            case Qt::Key_P: return InputKey::P;
            case Qt::Key_Q: return InputKey::Q;
            case Qt::Key_R: return InputKey::R;
            case Qt::Key_S: return InputKey::S;
            case Qt::Key_T: return InputKey::T;
            case Qt::Key_U: return InputKey::U;
            case Qt::Key_V: return InputKey::V;
            case Qt::Key_W: return InputKey::W;
            case Qt::Key_X: return InputKey::X;
            case Qt::Key_Y: return InputKey::Y;
            case Qt::Key_Z: return InputKey::Z;

            case Qt::Key_0: return InputKey::Num0;
            case Qt::Key_1: return InputKey::Num1;
            case Qt::Key_2: return InputKey::Num2;
            case Qt::Key_3: return InputKey::Num3;
            case Qt::Key_4: return InputKey::Num4;
            case Qt::Key_5: return InputKey::Num5;
            case Qt::Key_6: return InputKey::Num6;
            case Qt::Key_7: return InputKey::Num7;
            case Qt::Key_8: return InputKey::Num8;
            case Qt::Key_9: return InputKey::Num9;

            case Qt::Key_Escape: return InputKey::Escape;
            case Qt::Key_Tab: return InputKey::Tab;
            case Qt::Key_CapsLock: return InputKey::CapsLock;
            case Qt::Key_Space: return InputKey::Space;
            case Qt::Key_Return:
            case Qt::Key_Enter:
                return InputKey::Enter;
            case Qt::Key_Backspace: return InputKey::Backspace;

            case Qt::Key_Insert: return InputKey::Insert;
            case Qt::Key_Delete: return InputKey::Delete;
            case Qt::Key_Home: return InputKey::Home;
            case Qt::Key_End: return InputKey::End;
            case Qt::Key_PageUp: return InputKey::PageUp;
            case Qt::Key_PageDown: return InputKey::PageDown;

            case Qt::Key_Up: return InputKey::ArrowUp;
            case Qt::Key_Down: return InputKey::ArrowDown;
            case Qt::Key_Left: return InputKey::ArrowLeft;
            case Qt::Key_Right: return InputKey::ArrowRight;

            case Qt::Key_F1: return InputKey::F1;
            case Qt::Key_F2: return InputKey::F2;
            case Qt::Key_F3: return InputKey::F3;
            case Qt::Key_F4: return InputKey::F4;
            case Qt::Key_F5: return InputKey::F5;
            case Qt::Key_F6: return InputKey::F6;
            case Qt::Key_F7: return InputKey::F7;
            case Qt::Key_F8: return InputKey::F8;
            case Qt::Key_F9: return InputKey::F9;
            case Qt::Key_F10: return InputKey::F10;
            case Qt::Key_F11: return InputKey::F11;
            case Qt::Key_F12: return InputKey::F12;
            default:
                break;
            }

            // Modifier left/right distinction usually needs native scancode.
            // These values are Windows set-1 scancodes.
            // You can expand this per-platform later.
#if defined(_WIN32)
            switch (nativeScanCode)
            {
            case 0x2A: return InputKey::ShiftLeft;
            case 0x36: return InputKey::ShiftRight;
            case 0x1D: return InputKey::CtrlLeft;
            case 0x11D: return InputKey::CtrlRight;
            case 0x38: return InputKey::AltLeft;
            case 0x138: return InputKey::AltRight;
            default:
                break;
            }
#endif

            if (qtKey == Qt::Key_Shift)
            {
                return InputKey::ShiftLeft;
            }

            if (qtKey == Qt::Key_Control)
            {
                return InputKey::CtrlLeft;
            }

            if (qtKey == Qt::Key_Alt)
            {
                return InputKey::AltLeft;
            }

            return std::nullopt;
        }

        void SetMouseButton(Qt::MouseButton button, bool down)
        {
            using NexusEngine::MouseButton;

            if (!m_inputState)
            {
                return;
            }

            switch (button)
            {
            case Qt::LeftButton:
                m_inputState->mouseButtons[static_cast<size_t>(MouseButton::Left)] = down;
                break;

            case Qt::RightButton:
                m_inputState->mouseButtons[static_cast<size_t>(MouseButton::Right)] = down;
                break;

            case Qt::MiddleButton:
                m_inputState->mouseButtons[static_cast<size_t>(MouseButton::Middle)] = down;
                break;

            case Qt::BackButton:
                m_inputState->mouseButtons[static_cast<size_t>(MouseButton::X1)] = down;
                break;

            case Qt::ForwardButton:
                m_inputState->mouseButtons[static_cast<size_t>(MouseButton::X2)] = down;
                break;

            default:
                break;
            }
        }
    };
}