#pragma once
#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <SDL.h>
#include <flecs.h>
#include <vector>
#include <memory>
#if defined(_WIN32)
#include <Windows.h>
#endif // _WIN32

#include "Scene.h"
#include "common/GraphicsRenderer.h"

#define REF(x) (void)(x)

namespace Diligent
{
    struct IDeviceContext;
    struct IRenderDevice;
    struct ISwapChain;
    class ImGuiImplDiligent;
}

namespace NexusEngine
{
    class Engine;

    class IGameApp
    {
    public:
        virtual ~IGameApp() = default;

        /// <summary>
        /// Called once at the start of the game. Use this to set up scenes, entities, etc.
        /// </summary>
        /// <param name="engine">Reference to the engine, for scene management and other operations</param>
        /// <returns>True if initialization succeeded and the game can start; false to abort.</returns>
        virtual void OnStartup(Engine& engine) { REF(engine); };
        virtual void OnShutdown(Engine& engine) { REF(engine); };
    };

    class Engine
    {
    public:
        /// <summary>
        /// Initializes the game engine. This must be called before starting the game loop.
        /// </summary>
        /// <param name="win">Abstracted info about the native window</param>
        /// <returns></returns>
        bool Initialize(const NativeWindow& win, std::unique_ptr<IGameApp> game);

        /// <summary>
        /// Cleans up resources used by the engine.
        /// </summary>
        void Shutdown();

        /// <summary>
        /// Starts game loop. This is a blocking call that will run until the process is terminated.
        /// </summary>
        void Start();
        void RunFrame(float dt);

        // Scene management
        Scene& CreateScene(const std::string& name);
        void RemoveScene(const std::string& name);
        Scene* FindScene(const std::string& name);
        Scene* ActiveScene() { return m_activeScene; }
        void SetActiveScene(const std::string& name);

        // ImGui system
       /* bool EnableImGui(SDL_Window* sdlWindow, bool docking, bool multiViewport);
        void DisableImGui();*/
        void ProcessSDLEvent(const SDL_Event& e);

    private:
        void Tick(float dt);
        //void BeginUiFrame(float dt);
        //void RenderUi(Diligent::IDeviceContext* ctx);

    private:
        bool m_initialized = false;
        bool m_started = false;
        bool m_shutdown = false;
        IGameApp* m_game = nullptr;
        GraphicsRenderer m_graphicsRenderer;
        std::vector<std::unique_ptr<Scene>> m_scenes;
        Scene* m_activeScene = nullptr;

        struct UiState
        {
            bool enabled = false;
            SDL_Window* sdlWindow = nullptr;
            Diligent::ImGuiImplDiligent* imguiDiligent = nullptr;
        } m_ui;
    };


} // namespace NexusEngine
