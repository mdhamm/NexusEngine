#pragma once
#include <vector>
#include <memory>
#include <SDL.h>
#include <flecs.h>
#include "Scene.h"

#if defined(_WIN32)
#include <Windows.h>
#endif // _WIN32

#define REF(x) (void)(x)

namespace Diligent
{
    class IDeviceContext;
    class IRenderDevice;
    class ISwapChain;
}

namespace NexusEngine
{

    struct NativeWindow
    {
#if defined(_WIN32)
        HWND hWnd = nullptr;
#endif
        int width = 0;
        int height = 0;
    };

    struct GfxContext
    {
        Diligent::RefCntAutoPtr<Diligent::IRenderDevice>  device;
        Diligent::RefCntAutoPtr<Diligent::IDeviceContext> ctx;
        Diligent::RefCntAutoPtr<Diligent::ISwapChain>     swapchain;
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

        // Scene management
        Scene& CreateScene(const std::string& name);
        void RemoveScene(const std::string& name);
        Scene* FindScene(const std::string& name);
        Scene* ActiveScene() { return m_activeScene; }
        void SetActiveScene(const std::string& name);

        // ImGui system
        bool EnableImGui(SDL_Window* sdlWindow, bool docking, bool multiViewport);
        void DisableImGui();
        void ProcessSDLEvent(const SDL_Event& e);

    private:
        void Tick(float dt);
        void BeginUiFrame(float dt);
        void RenderUi(Diligent::IDeviceContext* ctx);

    private:
        bool m_initialized = false;
        bool m_shutdown = false;
        IGameApp* m_game = nullptr;
        GfxContext gfx_;
        std::vector<std::unique_ptr<Scene>> scenes_;
        Scene* m_activeScene = nullptr;

        struct UiState
        {
            bool enabled = false;
            SDL_Window* sdlWindow = nullptr;
            class Diligent::ImGuiImplDiligent* imguiDiligent = nullptr;
        } ui_;
    };

    class IGameApp
    {
    public:
        virtual ~IGameApp() = default;

        /// <summary>
        /// Called once at the start of the game. Use this to set up scenes, entities, etc.
        /// </summary>
        /// <param name="engine">Reference to the engine, for scene management and other operations</param>
        /// <returns>True if initialization succeeded and the game can start; false to abort.</returns>
        virtual void OnStartup(Engine& engine) {};
        virtual void OnShutdown(Engine& engine) {};
    };

    bool CreateDeviceAndSwapchain(const NativeWindow& win, GfxContext& out);
    void BeginFrame(GfxContext& gfx, float r, float g, float b, float a);
    void EndFrame(GfxContext& gfx);

} // namespace NexusEngine
