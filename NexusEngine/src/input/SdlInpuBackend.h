#pragma once

#include "InputState.h"

#include <SDL.h>

namespace NexusEngine
{
    class SdlInputBackend final : public IInputBackend
    {
    public:
        void BeginFrame() override
        {
            if (m_inputState)
            {
                m_inputState->ClearFrameDeltas();
            }

            SDL_PumpEvents();

            if (!m_inputState)
            {
                return;
            }

            int keyCount = 0;
            const Uint8* keyboard = SDL_GetKeyboardState(&keyCount);

            SetKey(InputKey::A, keyboard, keyCount, SDL_SCANCODE_A);
            SetKey(InputKey::B, keyboard, keyCount, SDL_SCANCODE_B);
            SetKey(InputKey::C, keyboard, keyCount, SDL_SCANCODE_C);
            SetKey(InputKey::D, keyboard, keyCount, SDL_SCANCODE_D);
            SetKey(InputKey::E, keyboard, keyCount, SDL_SCANCODE_E);
            SetKey(InputKey::F, keyboard, keyCount, SDL_SCANCODE_F);
            SetKey(InputKey::G, keyboard, keyCount, SDL_SCANCODE_G);
            SetKey(InputKey::H, keyboard, keyCount, SDL_SCANCODE_H);
            SetKey(InputKey::I, keyboard, keyCount, SDL_SCANCODE_I);
            SetKey(InputKey::J, keyboard, keyCount, SDL_SCANCODE_J);
            SetKey(InputKey::K, keyboard, keyCount, SDL_SCANCODE_K);
            SetKey(InputKey::L, keyboard, keyCount, SDL_SCANCODE_L);
            SetKey(InputKey::M, keyboard, keyCount, SDL_SCANCODE_M);
            SetKey(InputKey::N, keyboard, keyCount, SDL_SCANCODE_N);
            SetKey(InputKey::O, keyboard, keyCount, SDL_SCANCODE_O);
            SetKey(InputKey::P, keyboard, keyCount, SDL_SCANCODE_P);
            SetKey(InputKey::Q, keyboard, keyCount, SDL_SCANCODE_Q);
            SetKey(InputKey::R, keyboard, keyCount, SDL_SCANCODE_R);
            SetKey(InputKey::S, keyboard, keyCount, SDL_SCANCODE_S);
            SetKey(InputKey::T, keyboard, keyCount, SDL_SCANCODE_T);
            SetKey(InputKey::U, keyboard, keyCount, SDL_SCANCODE_U);
            SetKey(InputKey::V, keyboard, keyCount, SDL_SCANCODE_V);
            SetKey(InputKey::W, keyboard, keyCount, SDL_SCANCODE_W);
            SetKey(InputKey::X, keyboard, keyCount, SDL_SCANCODE_X);
            SetKey(InputKey::Y, keyboard, keyCount, SDL_SCANCODE_Y);
            SetKey(InputKey::Z, keyboard, keyCount, SDL_SCANCODE_Z);

            SetKey(InputKey::Num0, keyboard, keyCount, SDL_SCANCODE_0);
            SetKey(InputKey::Num1, keyboard, keyCount, SDL_SCANCODE_1);
            SetKey(InputKey::Num2, keyboard, keyCount, SDL_SCANCODE_2);
            SetKey(InputKey::Num3, keyboard, keyCount, SDL_SCANCODE_3);
            SetKey(InputKey::Num4, keyboard, keyCount, SDL_SCANCODE_4);
            SetKey(InputKey::Num5, keyboard, keyCount, SDL_SCANCODE_5);
            SetKey(InputKey::Num6, keyboard, keyCount, SDL_SCANCODE_6);
            SetKey(InputKey::Num7, keyboard, keyCount, SDL_SCANCODE_7);
            SetKey(InputKey::Num8, keyboard, keyCount, SDL_SCANCODE_8);
            SetKey(InputKey::Num9, keyboard, keyCount, SDL_SCANCODE_9);

            SetKey(InputKey::Escape, keyboard, keyCount, SDL_SCANCODE_ESCAPE);
            SetKey(InputKey::Tab, keyboard, keyCount, SDL_SCANCODE_TAB);
            SetKey(InputKey::CapsLock, keyboard, keyCount, SDL_SCANCODE_CAPSLOCK);
            SetKey(InputKey::ShiftLeft, keyboard, keyCount, SDL_SCANCODE_LSHIFT);
            SetKey(InputKey::ShiftRight, keyboard, keyCount, SDL_SCANCODE_RSHIFT);
            SetKey(InputKey::CtrlLeft, keyboard, keyCount, SDL_SCANCODE_LCTRL);
            SetKey(InputKey::CtrlRight, keyboard, keyCount, SDL_SCANCODE_RCTRL);
            SetKey(InputKey::AltLeft, keyboard, keyCount, SDL_SCANCODE_LALT);
            SetKey(InputKey::AltRight, keyboard, keyCount, SDL_SCANCODE_RALT);
            SetKey(InputKey::Space, keyboard, keyCount, SDL_SCANCODE_SPACE);
            SetKey(InputKey::Enter, keyboard, keyCount, SDL_SCANCODE_RETURN);
            SetKey(InputKey::Backspace, keyboard, keyCount, SDL_SCANCODE_BACKSPACE);

            SetKey(InputKey::Insert, keyboard, keyCount, SDL_SCANCODE_INSERT);
            SetKey(InputKey::Delete, keyboard, keyCount, SDL_SCANCODE_DELETE);
            SetKey(InputKey::Home, keyboard, keyCount, SDL_SCANCODE_HOME);
            SetKey(InputKey::End, keyboard, keyCount, SDL_SCANCODE_END);
            SetKey(InputKey::PageUp, keyboard, keyCount, SDL_SCANCODE_PAGEUP);
            SetKey(InputKey::PageDown, keyboard, keyCount, SDL_SCANCODE_PAGEDOWN);

            SetKey(InputKey::ArrowUp, keyboard, keyCount, SDL_SCANCODE_UP);
            SetKey(InputKey::ArrowDown, keyboard, keyCount, SDL_SCANCODE_DOWN);
            SetKey(InputKey::ArrowLeft, keyboard, keyCount, SDL_SCANCODE_LEFT);
            SetKey(InputKey::ArrowRight, keyboard, keyCount, SDL_SCANCODE_RIGHT);

            SetKey(InputKey::F1, keyboard, keyCount, SDL_SCANCODE_F1);
            SetKey(InputKey::F2, keyboard, keyCount, SDL_SCANCODE_F2);
            SetKey(InputKey::F3, keyboard, keyCount, SDL_SCANCODE_F3);
            SetKey(InputKey::F4, keyboard, keyCount, SDL_SCANCODE_F4);
            SetKey(InputKey::F5, keyboard, keyCount, SDL_SCANCODE_F5);
            SetKey(InputKey::F6, keyboard, keyCount, SDL_SCANCODE_F6);
            SetKey(InputKey::F7, keyboard, keyCount, SDL_SCANCODE_F7);
            SetKey(InputKey::F8, keyboard, keyCount, SDL_SCANCODE_F8);
            SetKey(InputKey::F9, keyboard, keyCount, SDL_SCANCODE_F9);
            SetKey(InputKey::F10, keyboard, keyCount, SDL_SCANCODE_F10);
            SetKey(InputKey::F11, keyboard, keyCount, SDL_SCANCODE_F11);
            SetKey(InputKey::F12, keyboard, keyCount, SDL_SCANCODE_F12);

            int dx = 0;
            int dy = 0;
            const Uint32 mouseMask = SDL_GetRelativeMouseState(&dx, &dy);

            m_inputState->mouseDeltaX += dx;
            m_inputState->mouseDeltaY += dy;

            int mx = 0;
            int my = 0;
            SDL_GetMouseState(&mx, &my);

            m_inputState->mouseX = mx;
            m_inputState->mouseY = my;

            m_inputState->mouseButtons[static_cast<size_t>(MouseButton::Left)] =
                (mouseMask & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;

            m_inputState->mouseButtons[static_cast<size_t>(MouseButton::Right)] =
                (mouseMask & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;

            m_inputState->mouseButtons[static_cast<size_t>(MouseButton::Middle)] =
                (mouseMask & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;

            m_inputState->mouseButtons[static_cast<size_t>(MouseButton::X1)] =
                (mouseMask & SDL_BUTTON(SDL_BUTTON_X1)) != 0;

            m_inputState->mouseButtons[static_cast<size_t>(MouseButton::X2)] =
                (mouseMask & SDL_BUTTON(SDL_BUTTON_X2)) != 0;
        }

        void EndFrame() override
        {
        }

        void AddMouseWheelDelta(float delta)
        {
            if (!m_inputState)
            {
                return;
            }
            m_inputState.mouseWheelDelta += delta;
        }

    private:
        static void SetKey(
            InputKey key,
            const Uint8* keyboard,
            int keyCount,
            SDL_Scancode scanCode)
        {
            const int index = static_cast<int>(scanCode);
            if (index >= 0 && index < keyCount)
            {
                // This needs access to m_inputState, so this static version is not enough.
            }
        }

        void SetKey(
            InputKey key,
            const Uint8* keyboard,
            int keyCount,
            SDL_Scancode scanCode)
        {
            const int index = static_cast<int>(scanCode);
            if (index >= 0 && index < keyCount)
            {
                m_inputState.keys[static_cast<size_t>(key)] = keyboard[index] != 0;
            }
        }
    };
}