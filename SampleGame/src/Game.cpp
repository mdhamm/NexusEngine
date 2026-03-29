#include "Game.h"
#include <Scene.h>
#include <components/CameraComponent.h>
#include <components/TransformComponent.h>
#include <components/RenderMeshComponent.h>
#include <rendering/RenderResourceFactory.h>
#include <SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <flecs.h>

namespace SampleGame
{
    namespace
    {
        float Fade(float t)
        {
            return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
        }

        float Lerp(float a, float b, float t)
        {
            return a + t * (b - a);
        }

        float Grad(int hash, float x, float y)
        {
            switch (hash & 0x7)
            {
                case 0: return  x + y;
                case 1: return -x + y;
                case 2: return  x - y;
                case 3: return -x - y;
                case 4: return  x;
                case 5: return -x;
                case 6: return  y;
                default:return -y;
            }
        }

        float PerlinNoise2D(float x, float y)
        {
            static constexpr std::array<int, 256> Permutation = {
                151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
                140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,
                247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
                57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
                74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,
                60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
                65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,
                200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,
                52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,
                207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,
                119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,
                129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,
                218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,
                81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,
                184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,
                222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
            };

            static std::array<int, 512> p = []()
            {
                std::array<int, 512> values{};
                for (size_t i = 0; i < 256; ++i)
                {
                    values[i] = Permutation[i];
                    values[i + 256] = Permutation[i];
                }
                return values;
            }();

            const int xi = static_cast<int>(std::floor(x)) & 255;
            const int yi = static_cast<int>(std::floor(y)) & 255;
            const float xf = x - std::floor(x);
            const float yf = y - std::floor(y);

            const float u = Fade(xf);
            const float v = Fade(yf);

            const int aa = p[p[xi] + yi];
            const int ab = p[p[xi] + yi + 1];
            const int ba = p[p[xi + 1] + yi];
            const int bb = p[p[xi + 1] + yi + 1];

            const float x1 = Lerp(Grad(aa, xf, yf), Grad(ba, xf - 1.0f, yf), u);
            const float x2 = Lerp(Grad(ab, xf, yf - 1.0f), Grad(bb, xf - 1.0f, yf - 1.0f), u);
            return Lerp(x1, x2, v);
        }
    }

    // ---------- Components ----------
    struct FlyCameraControllerComponent
    {
        float m_moveSpeed = 12.0f;
        float m_lookSensitivity = 0.003f;
    };

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
            w.component<FlyCameraControllerComponent>()
                .member<float>("moveSpeed")
                .member<float>("lookSensitivity");
            w.component<RotationSpeed>().member<float>("x").member<float>("y").member<float>("z");

            SDL_SetRelativeMouseMode(SDL_TRUE);

            // Create a default camera
            auto camera = w.entity("MainCamera")
                .set(NexusEngine::CameraComponent{})
                .set(NexusEngine::TransformComponent{
                    .m_localPosition = Diligent::float3(0.0f, 6.0f, -18.0f),
                    .m_localRotation = Diligent::float3(-0.25f, 0.0f, 0.0f),
                    .m_localScale = Diligent::float3(1.0f, 1.0f, 1.0f)
                })
                .set(FlyCameraControllerComponent{});

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
                    constexpr int CubeCountX = 10;
                    constexpr int CubeCountZ = 10;
                    constexpr float Spacing = 0.35f;
                    constexpr float NoiseFrequency = 0.045f;
                    constexpr float HeightScale = 6.0f;
                    constexpr float BaseScale = 0.12f;

                    for (int z = 0; z < CubeCountZ; ++z)
                    {
                        for (int x = 0; x < CubeCountX; ++x)
                        {
                            const float sampleX = static_cast<float>(x) * NoiseFrequency;
                            const float sampleZ = static_cast<float>(z) * NoiseFrequency;
                            const float noise = PerlinNoise2D(sampleX, sampleZ);
                            const float height = noise * HeightScale;
                            const float heightScaleBias = noise > 0.0f ? noise * 0.15f : 0.0f;

                            auto noiseCube = w.entity()
                                .set(NexusEngine::TransformComponent{
                                    .m_localPosition = Diligent::float3(
                                        (static_cast<float>(x) - static_cast<float>(CubeCountX) * 0.5f) * Spacing,
                                        height,
                                        (static_cast<float>(z) - static_cast<float>(CubeCountZ) * 0.5f) * Spacing),
                                    .m_localRotation = Diligent::float3(0.0f, 0.0f, 0.0f),
                                    .m_localScale = Diligent::float3(BaseScale, BaseScale + heightScaleBias, BaseScale)
                                });

                            auto* noiseRenderMesh = noiseCube.get_mut<NexusEngine::RenderMeshComponent>();
                            noiseRenderMesh->mesh = cubeMesh;
                            noiseRenderMesh->material = unlitMaterial;
                            noiseRenderMesh->visible = true;
                        }
                    }

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

            w.system<NexusEngine::TransformComponent, FlyCameraControllerComponent, NexusEngine::CameraComponent>("UpdateFlyCamera")
                .kind(flecs::OnUpdate)
                .iter(
                    [](flecs::iter& it, NexusEngine::TransformComponent* transforms, FlyCameraControllerComponent* controllers, NexusEngine::CameraComponent*)
                    {
                        const float dt = static_cast<float>(it.delta_time());
                        const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);
                        int mouseDeltaX = 0;
                        int mouseDeltaY = 0;
                        SDL_GetRelativeMouseState(&mouseDeltaX, &mouseDeltaY);

                        for (auto i : it)
                        {
                            auto& transform = transforms[i];
                            const auto& controller = controllers[i];

                            transform.m_localRotation.y += static_cast<float>(mouseDeltaX) * controller.m_lookSensitivity;
                            transform.m_localRotation.x += static_cast<float>(mouseDeltaY) * controller.m_lookSensitivity;
                            transform.m_localRotation.x = std::clamp(transform.m_localRotation.x, -1.55f, 1.55f);

                            const Diligent::float4x4 rotationMatrix =
                                Diligent::float4x4::RotationZ(transform.m_localRotation.z) *
                                Diligent::float4x4::RotationY(transform.m_localRotation.y) *
                                Diligent::float4x4::RotationX(transform.m_localRotation.x);

                            const Diligent::float3 look = Diligent::normalize(Diligent::float3(0.0f, 0.0f, 1.0f) * rotationMatrix);
                            const Diligent::float3 up = Diligent::normalize(Diligent::float3(0.0f, 1.0f, 0.0f) * rotationMatrix);
                            const Diligent::float3 right = Diligent::normalize(Diligent::cross(up, look));

                            Diligent::float3 movement(0.0f, 0.0f, 0.0f);
                            if (keyboardState[SDL_SCANCODE_W])
                            {
                                movement += look;
                            }
                            if (keyboardState[SDL_SCANCODE_S])
                            {
                                movement -= look;
                            }
                            if (keyboardState[SDL_SCANCODE_D])
                            {
                                movement += right;
                            }
                            if (keyboardState[SDL_SCANCODE_A])
                            {
                                movement -= right;
                            }

                            if (Diligent::length(movement) > 0.0f)
                            {
                                movement = Diligent::normalize(movement) * (controller.m_moveSpeed * dt);
                                transform.m_localPosition += movement;
                            }
                        }
                    });

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
