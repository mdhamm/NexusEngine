#include "Game.h"
#include <Scene.h>
#include <components/CameraComponent.h>
#include <components/TransformComponent.h>
#include <components/RenderMeshComponent.h>
#include <rendering/RenderResourceFactory.h>
#include <flecs.h>
#include <cmath>

namespace SampleGame
{
    // ---------- Components ----------
    struct LocalTransform { float m_x = 0, m_y = 0, m_z = 0; };
    struct WorldTransform { float m_x = 0, m_y = 0, m_z = 0; };
    struct Velocity { float m_x = 0, m_y = 0, m_z = 0; };
    struct RotationSpeed { float m_x = 0, m_y = 0, m_z = 0; };

    class Game final : public NexusEngine::IGameApp
    {
    public:
        void OnStartup(NexusEngine::Engine& engine) override
        {
            auto* scene = &engine.CreateScene("MainScene");
            engine.SetActiveScene("MainScene");
            auto& w = scene->m_world;

            // Register components
            w.component<LocalTransform>().member<float>("x").member<float>("y").member<float>("z");
            w.component<WorldTransform>().member<float>("x").member<float>("y").member<float>("z");
            w.component<Velocity>().member<float>("x").member<float>("y").member<float>("z");
            w.component<RotationSpeed>().member<float>("x").member<float>("y").member<float>("z");

            // Create a default camera
            auto camera = w.entity("MainCamera")
                .set(NexusEngine::CameraComponent{});

            // Create rendering resources using the factory
            auto* factory = scene->GetResourceFactory();
            if (factory)
            {
                // Create shared resources
                // TODO: we are leaking these. Also why are we creating a material in the game??
                NexusEngine::Mesh* cubeMesh = factory->CreateCubeMesh();
                NexusEngine::Material* unlitMaterial = factory->CreateUnlitMaterial();

                if (cubeMesh && unlitMaterial)
                {
                    auto parentCube = w.entity("ParentCube")
                        .set(NexusEngine::TransformComponent{})
                        .set(RotationSpeed{ 0.35f, 0.7f, 0.0f });

                    auto* parentRenderMesh = parentCube.get_mut<NexusEngine::RenderMeshComponent>();
                    parentRenderMesh->mesh = cubeMesh;
                    parentRenderMesh->material = unlitMaterial;
                    parentRenderMesh->visible = true;

                    // Create rotating cube entity as a child of the parent cube
                    auto cube = w.entity("RotatingCube")
                        .child_of(parentCube)
                        .set(NexusEngine::TransformComponent{
                            .m_localPosition = Diligent::float3(1.0f, 0.0f, 0.0f),
                            .m_localRotation = Diligent::float3(0.0f, 0.0f, 0.0f),
                            .m_localScale = Diligent::float3(0.3f, 0.3f, 0.3f)
                        })
                        .set(RotationSpeed{ 0.0f, 1.0f, 0.5f });

                    auto* renderMesh = cube.get_mut<NexusEngine::RenderMeshComponent>();
                    renderMesh->mesh = cubeMesh;
                    renderMesh->material = unlitMaterial;
                    renderMesh->visible = true;
                }
            }

            // System to rotate objects with RotationSpeed component
            w.system<NexusEngine::TransformComponent, RotationSpeed>("RotateObjects")
                .kind(flecs::OnUpdate)
                .iter(
                    [](flecs::iter& it, NexusEngine::TransformComponent* transforms, RotationSpeed* speeds)
                    {
                        const float dt = static_cast<float>(it.delta_time());
                        for (auto i : it)
                        {
                            transforms[i].m_localRotation.x += speeds[i].m_x * dt;
                            transforms[i].m_localRotation.y += speeds[i].m_y * dt;
                            transforms[i].m_localRotation.z += speeds[i].m_z * dt;
                        }
                    });

            // 1) Hierarchy propagate: every frame
            w.system<LocalTransform, WorldTransform>("PropagateTransforms")
                .kind(flecs::OnUpdate)
                .each(
                    [](flecs::entity e, LocalTransform& local, WorldTransform& world)
                    {
                        if (auto p = e.parent(); p) {
                            if (const WorldTransform* pw = p.get<WorldTransform>())
                            {
                                world = { pw->m_x + local.m_x, pw->m_y + local.m_y, pw->m_z + local.m_z };
                                return;
                            }
                        }
                        world = { local.m_x, local.m_y, local.m_z };
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
                            lt[i].m_x += v[i].m_x * dt;
                            lt[i].m_y += v[i].m_y * dt;
                            lt[i].m_z += v[i].m_z * dt;
                        }
                    });

            // 3) AI: run every 0.1 seconds (NOT every frame)
            w.system<>("AI_Tick")
                .kind(flecs::OnUpdate)
                .interval(0.1f)
                .iter(
                    [](flecs::iter& it)
                    {
                        (void)it;
                    });

            // 4) Expensive culling: run once every 4 frames
            w.system<>("VisibilityCulling")
                .kind(flecs::PostUpdate)
                .rate(4)
                .iter(
                    [](flecs::iter& it)
                    {
                        (void)it;
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
