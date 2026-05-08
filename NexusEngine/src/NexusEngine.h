#pragma once

#include "Scene.h"
#include "input/InputState.h"
#include "rendering/GraphicsRenderer.h"

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <SDL.h>
#include <filesystem>
#include <flecs.h>
#include <memory>
#include <vector>

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

    // Game-facing interface implemented by runtime or sample code.
    class IGameApp
    {
    public:
        virtual ~IGameApp() = default;

        /// <summary>
        /// Called once during engine startup to create scenes and game state.
        /// </summary>
        /// <param name="engine">Engine instance used to create and configure game state.</param>
        virtual void OnStartup(Engine& engine) { REF(engine); };

        /// <summary>
        /// Called during shutdown before engine resources are released.
        /// </summary>
        /// <param name="engine">Engine instance being shut down.</param>
        virtual void OnShutdown(Engine& engine) { REF(engine); };
    };

    struct ProjectContext
    {
        std::filesystem::path m_projectRoot;

        std::filesystem::path GetAssetReferenceDirectory() const { return m_projectRoot / ".nexus" / "assetrefs"; }
    };

    // Main engine entry point that owns rendering and scene lifetime.
    class Engine
    {
    public:
        /// <summary>
        /// Initializes the engine against a native window and game instance.
        /// </summary>
        /// <param name="win">Native window description used for renderer startup.</param>
        /// <param name="game">Game implementation owned by the engine.</param>
        /// <param name="projectRoot">Root directory of the current project, used for asset loading.</param>
        /// <returns>True if initialization succeeds; otherwise false.</returns>
        bool Initialize(const NativeWindow& win, std::unique_ptr<IGameApp> game, std::filesystem::path projectRoot);

        /// <summary>
        /// Releases engine-owned resources and shuts the game down.
        /// </summary>
        void Shutdown();

        /// <summary>
        /// Runs the main blocking game loop.
        /// </summary>
        void Start();

        /// <summary>
        /// Advances the engine by a single frame.
        /// </summary>
        /// <param name="dt">Frame delta time in seconds.</param>
        void RunFrame(float dt);

        /// <summary>
        /// Creates and stores a named scene.
        /// </summary>
        /// <param name="name">Name assigned to the new scene.</param>
        /// <returns>A reference to the created scene.</returns>
        Scene& CreateScene(const std::string& name);

        /// <summary>
        /// Removes a scene by name.
        /// </summary>
        /// <param name="name">Name of the scene to remove.</param>
        void RemoveScene(const std::string& name);

        /// <summary>
        /// Finds a scene by name.
        /// </summary>
        /// <param name="name">Name of the scene to locate.</param>
        /// <returns>A pointer to the scene if found; otherwise null.</returns>
        Scene* FindScene(const std::string& name);

        /// <summary>
        /// Returns the currently active scene.
        /// </summary>
        /// <returns>The active scene, or null if none is active.</returns>
        Scene* ActiveScene() { return m_activeScene; }

        /// <summary>
        /// Sets the active scene by name.
        /// </summary>
        /// <param name="name">Name of the scene to activate.</param>
        void SetActiveScene(const std::string& name);

        /// <summary>
        /// Returns the current input state for the frame.
        /// </summary>
        /// <returns>Current frame input state.</returns>
        inline InputState& GetInputState() { return m_inputState; }

        /// <summary>
        /// Sets the input backend used to populate the input state each frame.
        /// </summary>
        /// <param name="backend">Input backend to use for the current game session.</param>
        inline void SetInputBackend(IInputBackend* backend) { m_inputBackend = backend; }

        /// <summary>
        /// Resizes the engine output surface.
        /// </summary>
        /// <param name="width">New output width in pixels.</param>
        /// <param name="height">New output height in pixels.</param>
        void ResizeOutput(int width, int height);

        const ProjectContext& GetProjectContext() const { return m_projectContext; }

    private:
        void Tick(float dt);
        //void BeginUiFrame(float dt);
        //void RenderUi(Diligent::IDeviceContext* ctx);

    private:
        // Tracks whether initialization completed successfully.
        bool m_initialized = false;

        // Tracks whether the blocking main loop has started.
        bool m_started = false;

        // Tracks whether shutdown has already been requested.
        bool m_shutdown = false;

        // Non-owning pointer to the active game implementation.
        IGameApp* m_game = nullptr;

        // Rendering backend used by the engine.
        GraphicsRenderer m_graphicsRenderer;

        // Owned scenes managed by the engine.
        std::vector<std::unique_ptr<Scene>> m_scenes;

        // Currently active scene.
        Scene* m_activeScene = nullptr;

        // Current frame's input state
        InputState m_inputState;

        IInputBackend* m_inputBackend = nullptr;

        ProjectContext m_projectContext;

        struct UiState
        {
            // Indicates whether UI rendering is active.
            bool enabled = false;

            // SDL window used by the UI backend.
            SDL_Window* sdlWindow = nullptr;

            // Diligent-backed ImGui renderer instance.
            Diligent::ImGuiImplDiligent* imguiDiligent = nullptr;
        } m_ui;
    };


} // namespace NexusEngine
