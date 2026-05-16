#pragma once

#include "Scene.h"
#include "GameLoopPhases.h"
#include "input/InputState.h"
#include "filesystem/AssetReferenceRegistry.h"
#include "rendering/GraphicsRenderer.h"
#include "rendering/RenderResourceFactory.h"

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <SDL.h>
#include <filesystem>
#include <flecs.h>
#include <functional>
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
    class MetadataRegistry;

    // Game-facing interface implemented by runtime or sample code.
    class IGameApp
    {
    public:
        virtual ~IGameApp() = default;

        /// <summary>
        /// Registers game-specific component metadata.
        /// </summary>
        /// <param name="world">Scene world receiving component registration.</param>
        /// <param name="metadataRegistry">Metadata registry receiving component metadata.</param>
        virtual void RegisterComponentMetadata(flecs::world& world, MetadataRegistry& metadataRegistry)
        {
            REF(world);
            REF(metadataRegistry);
        }

        /// <summary>
        /// Unregisters game-specific component metadata during unload.
        /// </summary>
        /// <param name="metadataRegistry">Metadata registry to remove game metadata from.</param>
        virtual void UnregisterComponentMetadata(MetadataRegistry& metadataRegistry)
        {
            REF(metadataRegistry);
        }

        /// <summary>
        /// Registers gameplay systems into the engine world and returns the created system entities.
        /// </summary>
        /// <param name="engine">Engine owning the world that receives the systems.</param>
        /// <returns>The created gameplay system entities.</returns>
        virtual std::vector<flecs::entity> RegisterSystems(Engine& engine)
        {
            REF(engine);
            return {};
        }

        /// <summary>
        /// Called once during engine startup to create scenes and game state.
        /// </summary>
        /// <param name="engine">Engine instance used to create and configure game state.</param>
        virtual void OnStartup(Engine& engine)
        {
            REF(engine);
        };

        /// <summary>
        /// Called during shutdown before engine resources are released.
        /// </summary>
        /// <param name="engine">Engine instance being shut down.</param>
        virtual void OnShutdown(Engine& engine)
        {
            REF(engine);
        };
    };

    // Main engine entry point that owns rendering and scene lifetime.
    class Engine
    {
    public:
        /// <summary>
        /// Initializes the engine against a native window.
        /// </summary>
        /// <param name="win">Native window description used for renderer startup.</param>
        /// <param name="projectRoot">Root directory of the current project, used for asset loading.</param>
        /// <returns>True if initialization succeeds; otherwise false.</returns>
        bool Initialize(const NativeWindow& win, std::filesystem::path projectRoot);

        /// <summary>
        /// Loads a game into the initialized engine.
        /// </summary>
        /// <param name="game">Game implementation to load.</param>
        /// <returns>True if the game was loaded; otherwise false.</returns>
        bool LoadGame(IGameApp& game);

        /// <summary>
        /// Unloads the current game from the engine.
        /// </summary>
        void UnloadGame();

        /// <summary>
        /// Releases engine-owned resources and shuts the game down.
        /// </summary>
        void Shutdown();

        /// <summary>
        /// Returns whether the engine has been initialized.
        /// </summary>
        /// <returns>True if the engine is initialized; otherwise false.</returns>
        bool IsInitialized() const { return m_initialized; }

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
        /// Loads the project's configured default scene into the engine.
        /// </summary>
        /// <returns>True if the default scene was loaded; otherwise false.</returns>
        bool LoadDefaultScene();

        /// <summary>
        /// Creates a scene owned by the engine.
        /// </summary>
        /// <param name="name">Name of the scene to create.</param>
        /// <returns>A reference to the created scene.</returns>
        Scene& CreateScene(const std::string& name);

        /// <summary>
        /// Removes a scene.
        /// </summary>
        /// <param name="scene">Scene to remove.</param>
        void RemoveScene(Scene& scene);

        /// <summary>
        /// Returns the currently active scene.
        /// </summary>
        /// <returns>The active scene, or null if none is active.</returns>
        Scene* ActiveScene() { return m_activeScene; }

        /// <summary>
        /// Sets the active scene.
        /// </summary>
        /// <param name="scene">Scene to activate.</param>
        void SetActiveScene(Scene& scene);

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

        const std::filesystem::path& GetProjectRoot() const { return m_projectRoot; }

        IO::AssetReferenceRegistry* GetAssetReferenceRegistry() { return m_assetReferenceRegistry; }

        flecs::world& GetWorld() { return m_world; }

        RenderResourceFactory* GetResourceFactory() { return m_resourceFactory.get(); }

    private:
        void Tick(float dt);
        void RegisterEngineWorldState();
        void RegisterEngineSystems();
        void RegisterGameSystems();
        void UnregisterGameSystems();
        bool EnsureInstanceTransformBufferCapacity(Diligent::Uint32 instanceCount);

    private:
        // Tracks whether initialization completed successfully.
        bool m_initialized = false;

        // Tracks whether the blocking main loop has started.
        bool m_started = false;

        // Tracks whether shutdown has already been requested.
        bool m_shutdown = false;

        // Non-owning pointer to the active game implementation.
        IGameApp* m_game = nullptr;

        bool m_gameStarted = false;

        // Rendering backend used by the engine.
        GraphicsRenderer m_graphicsRenderer;

        // Flecs world for scene and engine-level systems
        flecs::world m_world;

        // Owned scenes managed by the engine.
        std::vector<std::unique_ptr<Scene>> m_scenes;

        // Currently active scene.
        Scene* m_activeScene = nullptr;

        // Current frame's input state
        InputState m_inputState;

        IInputBackend* m_inputBackend = nullptr;

        std::filesystem::path m_projectRoot;

        IO::AssetReferenceRegistry* m_assetReferenceRegistry = nullptr;
        std::vector<flecs::entity_t> m_registeredGameSystemIds;

        std::unique_ptr<RenderResourceFactory> m_resourceFactory;

        Diligent::RefCntAutoPtr<Diligent::IBuffer> m_instanceTransformBuffer;
        Diligent::Uint32 m_instanceTransformCapacity = 0;

        float m_clearAnimationTime = 0.0f;

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
