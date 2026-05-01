#pragma once

#include <array>
#include <cstdint>
#include <algorithm>

namespace NexusEngine
{
    enum class InputKey : uint16_t
    {
        Unknown = 0,

        A, B, C, D, E, F, G,
        H, I, J, K, L, M, N,
        O, P, Q, R, S, T, U,
        V, W, X, Y, Z,

        Num0, Num1, Num2, Num3, Num4,
        Num5, Num6, Num7, Num8, Num9,

        Escape,
        Tab,
        CapsLock,
        ShiftLeft,
        ShiftRight,
        CtrlLeft,
        CtrlRight,
        AltLeft,
        AltRight,
        Space,
        Enter,
        Backspace,

        Insert,
        Delete,
        Home,
        End,
        PageUp,
        PageDown,

        ArrowUp,
        ArrowDown,
        ArrowLeft,
        ArrowRight,

        F1, F2, F3, F4, F5, F6,
        F7, F8, F9, F10, F11, F12,

        Count
    };

    enum class MouseButton : uint8_t
    {
        Left = 0,
        Right,
        Middle,
        X1,
        X2,

        Count
    };

    struct InputState
    {
        std::array<bool, static_cast<size_t>(InputKey::Count)> keys{};
        std::array<bool, static_cast<size_t>(MouseButton::Count)> mouseButtons{};

        int mouseX = 0;
        int mouseY = 0;

        int mouseDeltaX = 0;
        int mouseDeltaY = 0;

        float mouseWheelDelta = 0.0f;

        bool IsKeyDown(InputKey key) const
        {
            return keys[static_cast<size_t>(key)];
        }

        bool IsMouseDown(MouseButton button) const
        {
            return mouseButtons[static_cast<size_t>(button)];
        }

        void ClearFrameDeltas()
        {
            mouseDeltaX = 0;
            mouseDeltaY = 0;
            mouseWheelDelta = 0.0f;
        }
    };

    class IInputBackend
    {
    public:
        void SetInputState(InputState* inputState)
        {
            m_inputState = inputState;
        }

        virtual ~IInputBackend() = default;

        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;

    protected:
        InputState* m_inputState = nullptr;
    };
}