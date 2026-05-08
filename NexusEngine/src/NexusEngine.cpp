#include "NexusEngine.h"

#include "ProjectSettings.h"
#include "Scene.h"
#include "filesystem/FileIO.h"
#include "serialization/ProjectSettingsSerialization.h"

#include <chrono>
#include <algorithm>

using namespace Diligent;

namespace NexusEngine
{
    bool Engine::Initialize(const NativeWindow& win, std::unique_ptr<IGameApp> game, std::filesystem::path projectRoot)
    {
        // Initialize project context
        m_projectContext.m_projectRoot = projectRoot;

        // Store game instance
        assert(game);
        if (!game)
        {
            return false;
        }
        m_game = game.release();

        // Initialize graphics renderer
        bool graphicsInitialized = m_graphicsRenderer.CreateDeviceAndSwapchain(win);
        m_initialized = graphicsInitialized;
        return graphicsInitialized;
    }

    void Engine::Start()
    {
        assert(m_initialized);
        if (!m_initialized)
        {
            return;
        }

        // Game loop
        using clock = std::chrono::high_resolution_clock;
        auto lastTime = clock::now();
        while (!m_shutdown)
        {
            auto now = clock::now();
            std::chrono::duration<float> delta = now - lastTime;
            lastTime = now;
            RunFrame(delta.count());
        }
    }

    void Engine::RunFrame(float dt)
    {
        assert(m_initialized);
        if (!m_initialized || m_shutdown)
        {
            return;
        }

        if (!m_started)
        {
            m_started = true;
            if (m_game)
            {
                m_game->OnStartup(*this);
            }
        }

        Tick(dt);
    }

    void Engine::Shutdown()
    {
        assert(m_initialized);
        if (!m_initialized)
        {
            return;
        }

        m_scenes.clear();

        if (m_started && m_game)
        {
            m_game->OnShutdown(*this);
            m_started = false;
        }
    }

    Scene& Engine::CreateScene(const std::string& name)
    {
        auto s = std::make_unique<Scene>(m_graphicsRenderer, *this, name);
        Scene& ref = *s;
        m_scenes.push_back(std::move(s));
        return ref;
    }

    void Engine::RemoveScene(const std::string& name)
    {
        m_scenes.erase(std::remove_if(m_scenes.begin(), m_scenes.end(),
            [&](auto& sc)
            {
                return sc->m_name == name;
            }),
            m_scenes.end());
    }

    Scene* Engine::FindScene(const std::string& name)
    {
        for (auto& s : m_scenes)
        {
            if (s->m_name == name)
            {
                return s.get();
            }
        }
        return nullptr;
    }

    void Engine::SetActiveScene(const std::string& name)
    {
        if (auto* s = FindScene(name))
        {
            m_activeScene = s;
        }
    }

    void Engine::ResizeOutput(int width, int height)
    {
        if (!m_initialized)
        {
            return;
        }

        m_graphicsRenderer.ResizeSwapchain(width, height);
    }

    void Engine::Tick(float dt)
    {
        if (m_inputBackend)
        {
            m_inputBackend->BeginFrame();
            m_inputBackend->EndFrame();
        }

        if (m_activeScene)
        {
            m_activeScene->Update(dt);
        }
    }
} // namespace NexusEngine
