#include "Game.h"
#include <flecs.h>
#include <cmath>

namespace SampleGame
{
    // ---------- Components ----------
    struct LocalTransform { float x = 0, y = 0, z = 0; };
    struct WorldTransform { float x = 0, y = 0, z = 0; };
    struct Velocity { float x = 0, y = 0, z = 0; };

    class Game final : public NexusEngine::IGameApp
    {
    public:
        void OnStartup(NexusEngine::Engine& engine) override
        {
            auto& w = engine.CreateScene("MainScene").m_world;
            engine.SetActiveScene("MainScene"); // TODO: create version of SetActiveScene that takes a reference instead of looking up by name

            // Register components (for readable dashboards/inspector)
            w.component<LocalTransform>().member<float>("x").member<float>("y").member<float>("z");
            w.component<WorldTransform>().member<float>("x").member<float>("y").member<float>("z");
            w.component<Velocity>().member<float>("x").member<float>("y").member<float>("z");

            // TODO: the engine should be doing this

            // 1) Hierarchy propagate: every frame
            w.system<LocalTransform, WorldTransform>("PropagateTransforms")
                .kind(flecs::OnUpdate)
                .each(
                    [](flecs::entity e, LocalTransform& local, WorldTransform& world)
                    {
                        if (auto p = e.parent(); p) {
                            if (const WorldTransform* pw = p.get<WorldTransform>())
                            {
                                world = { pw->x + local.x, pw->y + local.y, pw->z + local.z };
                                return;
                            }
                        }
                        world = { local.x, local.y, local.z };
                    });

            // 2) Movement: every frame, uses dt from iterator
            w.system<LocalTransform, Velocity>("IntegrateVelocity")
                .kind(flecs::OnUpdate)
                .iter(
                    [](flecs::iter& it, LocalTransform* lt, Velocity* v)
                    {
                        const float dt = static_cast<float>(it.delta_time());
                        for (auto i : it)
                        {
                            lt[i].x += v[i].x * dt;
                            lt[i].y += v[i].y * dt;
                            lt[i].z += v[i].z * dt;
                        }
                    });

            // 3) AI: run every 0.1 seconds (NOT every frame)
            w.system<>("AI_Tick")
                .kind(flecs::OnUpdate)
                .interval(0.1f)          // <-- runs ~10 Hz
                .iter(
                    [](flecs::iter& it) // payload optional; using .run keeps a no-component system
                    {
                        (void)it;
                        // Query AI, pathfind, etc.
                    });

            // 4) Expensive culling: run once every 4 frames
            w.system<>("VisibilityCulling")
                .kind(flecs::PostUpdate)
                .rate(4)                 // <-- runs at 1/4 frame rate
                .iter(
                    [](flecs::iter& it)
                    {
                        (void)it;
                        // Broadphase/visibility here
                    });

            // Create a small hierarchy to see propagation working
            auto root = w.entity("Root")
                .set(LocalTransform{ 0,0,0 })
                .set(WorldTransform{ 0,0,0 });

            auto child = w.entity("Child")
                .child_of(root)
                .set(LocalTransform{ 1,0,0 })
                .set(WorldTransform{ 0,0,0 })
                .set(Velocity{ 0.25f, 0, 0 });

            w.entity("Grandchild")
                .child_of(child)
                .set(LocalTransform{ 0,0.5f,0 })
                .set(WorldTransform{ 0,0,0 });
        }

        void OnShutdown(NexusEngine::Engine&) override
        {
        }
    };

    std::unique_ptr<NexusEngine::IGameApp> CreateGame()
    {
        return std::make_unique<Game>();
    }
} // namespace SampleGame
