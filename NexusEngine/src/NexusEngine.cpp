#include "NexusEngine.h"

#include "ProjectSettings.h"
#include "components/CameraComponent.h"
#include "components/EditorOnlyComponent.h"
#include "components/FlyCameraComponent.h"
#include "components/RenderMeshComponent.h"
#include "components/TransformComponent.h"
#include "filesystem/FileIO.h"
#include "reflection/EntityReflection.h"
#include "rendering/Material.h"
#include "rendering/Mesh.h"
#include "rendering/RenderResourceFactory.h"
#include "serialization/ProjectSettingsSerialization.h"
#include "serialization/SceneSerialization.h"

#include <DiligentCore/Common/interface/BasicMath.hpp>
#include <DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h>
#include <DiligentCore/Graphics/GraphicsTools/interface/MapHelper.hpp>

#include <SDL.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <unordered_map>

using namespace Diligent;

namespace NexusEngine
{
    namespace
    {
        std::filesystem::path GetProjectSettingsPath(const std::filesystem::path& projectRoot)
        {
            return projectRoot / "NexusProject.json";
        }

        void RegisterBuiltinComponents(flecs::world& world)
        {
            static bool s_registered = false;
            if (s_registered)
            {
                return;
            }

            MetadataRegistry& registry = MetadataRegistry::Instance();
            RegisterComponent<TransformComponent>(world, registry);
            RegisterComponent<CameraComponent>(world, registry);
            RegisterComponent<RenderTextureComponent>(world, registry);
            RegisterComponent<RenderMeshComponent>(world, registry);
            RegisterComponent<FlyCameraComponent>(world, registry);

            s_registered = true;
        }

        struct InstanceTransformData
        {
            float4 m_col0;
            float4 m_col1;
            float4 m_col2;
            float4 m_col3;
        };

        struct InstancingBatchKey
        {
            Mesh* m_mesh = nullptr;
            Material* m_material = nullptr;

            bool operator==(const InstancingBatchKey& other) const = default;
        };

        struct InstancingBatchKeyHasher
        {
            size_t operator()(const InstancingBatchKey& key) const noexcept
            {
                const size_t meshHash = std::hash<std::uintptr_t>{}(reinterpret_cast<std::uintptr_t>(key.m_mesh));
                const size_t materialHash = std::hash<std::uintptr_t>{}(reinterpret_cast<std::uintptr_t>(key.m_material));
                return (meshHash * 1315423911u) ^ materialHash;
            }
        };

        float4x4 CreateRightHandedProjectionMatrix(float fov, float aspectRatio, float zNear, float zFar)
        {
            const float yScale = 1.0f / std::tan(fov * 0.5f);
            const float xScale = yScale / aspectRatio;

            float4x4 projection = float4x4::Identity();
            projection._11 = xScale;
            projection._22 = yScale;
            projection._33 = zFar / (zNear - zFar);
            projection._34 = -1.0f;
            projection._43 = (zNear * zFar) / (zNear - zFar);
            projection._44 = 0.0f;
            return projection;
        }

        InstanceTransformData CreateInstanceTransformData(const TransformComponent* transform)
        {
            const float4x4 world = transform ? transform->GetWorldMatrix() : float4x4::Identity();

            InstanceTransformData instanceData{};
            instanceData.m_col0 = float4(world.m[0][0], world.m[1][0], world.m[2][0], world.m[3][0]);
            instanceData.m_col1 = float4(world.m[0][1], world.m[1][1], world.m[2][1], world.m[3][1]);
            instanceData.m_col2 = float4(world.m[0][2], world.m[1][2], world.m[2][2], world.m[3][2]);
            instanceData.m_col3 = float4(world.m[0][3], world.m[1][3], world.m[2][3], world.m[3][3]);
            return instanceData;
        }
    }

    bool Engine::Initialize(const NativeWindow& win, std::filesystem::path projectRoot)
    {
        m_projectRoot = std::move(projectRoot);

        bool success = true;

        m_assetReferenceRegistry = new IO::AssetReferenceRegistry(m_projectRoot);
        success &= m_graphicsRenderer.CreateDeviceAndSwapchain(win);
        if (success && m_graphicsRenderer.m_gfx.m_device && m_graphicsRenderer.m_gfx.m_ctx)
        {
            m_resourceFactory = std::make_unique<RenderResourceFactory>(
                m_graphicsRenderer.m_gfx.m_device,
                m_graphicsRenderer.m_gfx.m_ctx);
        }

        RegisterBuiltinComponents(m_world);

        RegisterEngineWorldState();
        RegisterEngineSystems();

        assert(success);
        m_initialized = success;
        return success;
    }

    bool Engine::LoadGame(IGameApp& game)
    {
        if (!m_initialized || m_game == &game)
        {
            return m_initialized && m_game == &game;
        }

        UnloadGame();

        m_game = &game;
        m_game->RegisterComponentMetadata(m_world, MetadataRegistry::Instance());
        RegisterGameSystems();
        return true;
    }

    void Engine::UnloadGame()
    {
        if (!m_game)
        {
            return;
        }

        if (m_gameStarted)
        {
            m_game->OnShutdown(*this);
            m_gameStarted = false;
        }

        UnregisterGameSystems();
        m_game->UnregisterComponentMetadata(MetadataRegistry::Instance());
        m_game = nullptr;
    }

    void Engine::Shutdown()
    {
        assert(m_initialized);
        if (!m_initialized)
        {
            return;
        }

        m_scenes.clear();
        m_activeScene = nullptr;
        UnloadGame();
        m_started = false;

        if (m_assetReferenceRegistry)
        {
            delete m_assetReferenceRegistry;
            m_assetReferenceRegistry = nullptr;
        }

        m_resourceFactory.reset();
        m_initialized = false;
    }

    void Engine::Start()
    {
        assert(m_initialized);
        if (!m_initialized)
        {
            return;
        }

        using clock = std::chrono::high_resolution_clock;
        auto lastTime = clock::now();
        while (!m_shutdown)
        {
            const auto now = clock::now();
            const std::chrono::duration<float> delta = now - lastTime;
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
            if (m_game && !m_gameStarted)
            {
                m_game->OnStartup(*this);
                m_gameStarted = true;
            }

            if (!m_activeScene)
            {
                (void)LoadDefaultScene();
            }
        }

        Tick(dt);
    }

    bool Engine::LoadDefaultScene()
    {
        if (m_projectRoot.empty() || !m_assetReferenceRegistry)
        {
            return false;
        }

        ProjectSettings projectSettings;
        if (!IO::LoadFromFile(projectSettings, GetProjectSettingsPath(m_projectRoot), IO::FileFormat::Json))
        {
            return false;
        }

        if (projectSettings.m_defaultScene.IsEmpty())
        {
            return false;
        }

        const std::filesystem::path relativeScenePath = m_assetReferenceRegistry->ResolveAssetReferencePath(projectSettings.m_defaultScene);
        if (relativeScenePath.empty())
        {
            return false;
        }

        Scene& sceneRef = CreateScene(relativeScenePath.stem().string());
        if (!IO::LoadFromFile(sceneRef, m_projectRoot / relativeScenePath, IO::FileFormat::Json))
        {
            RemoveScene(sceneRef);
            return false;
        }

        SetActiveScene(sceneRef);
        return true;
    }

    Scene& Engine::CreateScene(const std::string& name)
    {
        auto scene = std::make_unique<Scene>(*this, name);
        Scene& sceneRef = *scene;
        m_scenes.push_back(std::move(scene));
        return sceneRef;
    }

    void Engine::RemoveScene(Scene& scene)
    {
        if (m_activeScene == &scene)
        {
            m_activeScene = nullptr;
        }

        m_scenes.erase(
            std::remove_if(
                m_scenes.begin(),
                m_scenes.end(),
                [&](const std::unique_ptr<Scene>& ownedScene)
                {
                    return ownedScene.get() == &scene;
                }),
            m_scenes.end());
    }

    void Engine::SetActiveScene(Scene& scene)
    {
        m_activeScene = &scene;
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

        m_world.progress(dt);
    }

    void Engine::RegisterEngineWorldState()
    {
        m_world.component<GameplayPhase>()
            .add(flecs::Phase)
            .depends_on(flecs::PreUpdate);

        m_world.component<PhysicsPhase>()
            .add(flecs::Phase)
            .depends_on<GameplayPhase>();

        m_world.component<TransformPhase>()
            .add(flecs::Phase)
            .depends_on<PhysicsPhase>();

        m_world.component<AnimationPhase>()
            .add(flecs::Phase)
            .depends_on<TransformPhase>();

        m_world.component<VisibilityPhase>()
            .add(flecs::Phase)
            .depends_on<AnimationPhase>();

        m_world.component<RenderPrepPhase>()
            .add(flecs::Phase)
            .depends_on<VisibilityPhase>();

        m_world.component<RenderPhase>()
            .add(flecs::Phase)
            .depends_on<RenderPrepPhase>();

        m_world.component<RenderPostPhase>()
            .add(flecs::Phase)
            .depends_on<RenderPhase>();
    }

    void Engine::RegisterEngineSystems()
    {
        m_world.system<>("BeginFrame")
            .kind<RenderPrepPhase>()
            .iter(
                [this](flecs::iter& it)
                {
                    m_clearAnimationTime += static_cast<float>(it.delta_time());
                    const float r = 0.10f + 0.05f * std::sin(m_clearAnimationTime * 1.7f);
                    const float g = 0.12f + 0.05f * std::sin(m_clearAnimationTime * 1.3f + 1.0f);
                    const float b = 0.16f + 0.05f * std::sin(m_clearAnimationTime * 0.9f + 2.0f);
                    m_graphicsRenderer.BeginFrame(r, g, b, 1.0f);
                });

        m_world.system<TransformComponent, FlyCameraComponent, CameraComponent>("UpdateFlyCamera")
            .kind<GameplayPhase>()
            .iter(
                [engine = this](flecs::iter& it, TransformComponent* transforms, FlyCameraComponent* controllers, CameraComponent*)
                {
                    const float dt = static_cast<float>(it.delta_time());
                    int mouseDeltaX = 0;
                    int mouseDeltaY = 0;
                    SDL_GetRelativeMouseState(&mouseDeltaX, &mouseDeltaY);

                    for (auto i : it)
                    {
                        flecs::entity entity = it.entity(i);
                        auto& transform = transforms[i];
                        auto& controller = controllers[i];

                        if (!controller.m_isRotationInitialized)
                        {
                            const float3 initialEuler = Quaternion::ToEuler(transform.GetWorldRotation());
                            controller.m_pitch = initialEuler.x;
                            controller.m_yaw = initialEuler.y;
                            controller.m_isRotationInitialized = true;
                        }

                        const float deltaYaw = -static_cast<float>(mouseDeltaX) * controller.m_lookSensitivity;
                        const float deltaPitch = -static_cast<float>(mouseDeltaY) * controller.m_lookSensitivity;
                        constexpr float MaxPitch = PI_F * 0.5f - 0.01f;

                        controller.m_yaw += deltaYaw;
                        controller.m_pitch = std::clamp(controller.m_pitch + deltaPitch, -MaxPitch, MaxPitch);

                        Quaternion worldRotation = Quaternion::FromEuler(0.0f, controller.m_yaw, 0.0f);
                        worldRotation = Quaternion::Multiply(worldRotation, Quaternion::FromEuler(controller.m_pitch, 0.0f, 0.0f));
                        const float3 forward = Quaternion::Foward(worldRotation);
                        const float3 right = Quaternion::Right(worldRotation);
                        SetWorldRotation(entity, worldRotation);

                        const InputState& input = engine->GetInputState();
                        float3 movement(0.0f, 0.0f, 0.0f);
                        if (input.IsKeyDown(InputKey::W)) movement += forward;
                        if (input.IsKeyDown(InputKey::S)) movement -= forward;
                        if (input.IsKeyDown(InputKey::D)) movement += right;
                        if (input.IsKeyDown(InputKey::A)) movement -= right;

                        if (length(movement) > 0.0f)
                        {
                            movement = normalize(movement) * (controller.m_moveSpeed * dt);
                            SetWorldPosition(entity, transform.GetWorldPosition() + movement);
                        }
                    }
                });

        m_world.system<CameraComponent>("RenderScene")
            .kind<RenderPhase>()
            .each(
                [this](flecs::entity cameraEntity, CameraComponent& cam)
                {
                    auto& ctx = m_graphicsRenderer.m_gfx.m_ctx;
                    auto& swapchain = m_graphicsRenderer.m_gfx.m_swapchain;
                    if (!ctx || !swapchain)
                    {
                        return;
                    }

                    const auto& scDesc = swapchain->GetDesc();
                    const float aspect = static_cast<float>(scDesc.Width) / static_cast<float>(scDesc.Height);
                    const auto rtvFormat = scDesc.ColorBufferFormat;
                    const auto dsvFormat = scDesc.DepthBufferFormat;
                    const bool gameplayEnabled = m_world.has<GameplayEnabled>();
                    const bool physicsEnabled = m_world.has<PhysicsEnabled>();
                    const bool sceneModeEnabled = !gameplayEnabled && !physicsEnabled;
                    const bool isEditorOnlyCamera = cameraEntity.has<EditorOnlyComponent>();

                    if (sceneModeEnabled && !isEditorOnlyCamera)
                    {
                        return;
                    }

                    if (!sceneModeEnabled && isEditorOnlyCamera)
                    {
                        return;
                    }

                    if (cam.m_target != CameraComponent::Target::SwapChain)
                    {
                        return;
                    }

                    float4x4 viewMatrix = float4x4::Translation(0.0f, 0.0f, -5.0f);
                    if (const auto* cameraTransform = cameraEntity.get<TransformComponent>())
                    {
                        viewMatrix = cameraTransform->GetWorldMatrix().Inverse();
                    }

                    const float4x4 projMatrix = CreateRightHandedProjectionMatrix(PI_F / 4.0f, aspect, 0.1f, 1000.0f);
                    const float4x4 viewProjMatrix = viewMatrix * projMatrix;

                    using BatchMap = std::unordered_map<InstancingBatchKey, std::vector<InstanceTransformData>, InstancingBatchKeyHasher>;
                    struct SingleDraw
                    {
                        Mesh* m_mesh = nullptr;
                        Material* m_material = nullptr;
                        IShaderResourceBinding* m_srb = nullptr;
                        InstanceTransformData m_instanceData{};
                    };

                    BatchMap instancedBatches;
                    std::vector<SingleDraw> singleDraws;
                    m_world.each<RenderMeshComponent>(
                        [&](flecs::entity meshEntity, RenderMeshComponent& renderMesh)
                        {
                            if (!renderMesh.visible || !renderMesh.mesh || !renderMesh.material)
                            {
                                return;
                            }

                            const InstanceTransformData instanceData = CreateInstanceTransformData(meshEntity.get<TransformComponent>());
                            if (renderMesh.instanceSRB || renderMesh.instanceConstantBuffer)
                            {
                                singleDraws.push_back(SingleDraw{ renderMesh.mesh, renderMesh.material, renderMesh.instanceSRB.RawPtr(), instanceData });
                                return;
                            }

                            instancedBatches[InstancingBatchKey{ renderMesh.mesh, renderMesh.material }].push_back(instanceData);
                        });

                    auto drawInstances =
                        [&](Mesh* mesh, Material* material, IShaderResourceBinding* explicitSrb, const InstanceTransformData* instanceData, Uint32 instanceCount)
                        {
                            if (!mesh || !material || !instanceData || instanceCount == 0)
                            {
                                return;
                            }

                            auto* cachedPipeline = m_resourceFactory
                                ? m_resourceFactory->GetOrCreatePipeline(material, mesh, rtvFormat, dsvFormat)
                                : nullptr;
                            if (!cachedPipeline || !cachedPipeline->m_pipelineState || !mesh->vertexBuffer)
                            {
                                return;
                            }

                            IShaderResourceBinding* srb = explicitSrb
                                ? explicitSrb
                                : cachedPipeline->m_shaderResourceBindingTemplate.RawPtr();
                            if (!srb || !EnsureInstanceTransformBufferCapacity(instanceCount))
                            {
                                return;
                            }

                            if (material->materialConstantBuffer)
                            {
                                struct VSConstantsData
                                {
                                    float4 col0;
                                    float4 col1;
                                    float4 col2;
                                    float4 col3;
                                };

                                MapHelper<VSConstantsData> mappedData(ctx, material->materialConstantBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
                                mappedData->col0 = float4(viewProjMatrix.m[0][0], viewProjMatrix.m[1][0], viewProjMatrix.m[2][0], viewProjMatrix.m[3][0]);
                                mappedData->col1 = float4(viewProjMatrix.m[0][1], viewProjMatrix.m[1][1], viewProjMatrix.m[2][1], viewProjMatrix.m[3][1]);
                                mappedData->col2 = float4(viewProjMatrix.m[0][2], viewProjMatrix.m[1][2], viewProjMatrix.m[2][2], viewProjMatrix.m[3][2]);
                                mappedData->col3 = float4(viewProjMatrix.m[0][3], viewProjMatrix.m[1][3], viewProjMatrix.m[2][3], viewProjMatrix.m[3][3]);
                            }

                            MapHelper<InstanceTransformData> mappedInstances(ctx, m_instanceTransformBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
                            std::memcpy(mappedInstances, instanceData, sizeof(InstanceTransformData) * instanceCount);
                            if (auto* instanceVar = srb->GetVariableByName(SHADER_TYPE_VERTEX, "g_InstanceData"))
                            {
                                instanceVar->Set(m_instanceTransformBuffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
                            }

                            ctx->SetPipelineState(cachedPipeline->m_pipelineState);
                            ctx->CommitShaderResources(srb, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
                            IBuffer* vertexBuffers[] = { mesh->vertexBuffer };
                            Uint64 offsets[] = { 0 };
                            ctx->SetVertexBuffers(0, 1, vertexBuffers, offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

                            if (mesh->indexBuffer && mesh->indexCount > 0)
                            {
                                ctx->SetIndexBuffer(mesh->indexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
                                DrawIndexedAttribs drawAttrs;
                                drawAttrs.IndexType = VT_UINT32;
                                drawAttrs.NumIndices = mesh->indexCount;
                                drawAttrs.NumInstances = instanceCount;
                                drawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
                                ctx->DrawIndexed(drawAttrs);
                            }
                            else if (mesh->vertexCount > 0)
                            {
                                DrawAttribs drawAttrs;
                                drawAttrs.NumVertices = mesh->vertexCount;
                                drawAttrs.NumInstances = instanceCount;
                                drawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
                                ctx->Draw(drawAttrs);
                            }
                        };

                    for (auto& [batchKey, batchInstances] : instancedBatches)
                    {
                        drawInstances(batchKey.m_mesh, batchKey.m_material, nullptr, batchInstances.data(), static_cast<Uint32>(batchInstances.size()));
                    }

                    for (const SingleDraw& draw : singleDraws)
                    {
                        drawInstances(draw.m_mesh, draw.m_material, draw.m_srb, &draw.m_instanceData, 1);
                    }
                });

        m_world.system<>("EndFrame")
            .kind<RenderPostPhase>()
            .iter(
                [this](flecs::iter&)
                {
                    m_graphicsRenderer.EndFrame();
                });
    }

    void Engine::RegisterGameSystems()
    {
        m_registeredGameSystemIds.clear();
        if (!m_game)
        {
            return;
        }

        const std::vector<flecs::entity> systemEntities = m_game->RegisterSystems(*this);
        for (const flecs::entity& systemEntity : systemEntities)
        {
            if (systemEntity.is_valid())
            {
                m_registeredGameSystemIds.push_back(static_cast<flecs::entity_t>(systemEntity.id()));
            }
        }
    }

    void Engine::UnregisterGameSystems()
    {
        for (const flecs::entity_t systemId : m_registeredGameSystemIds)
        {
            flecs::entity systemEntity = m_world.entity(systemId);
            if (systemEntity.is_valid() && systemEntity.is_alive())
            {
                systemEntity.destruct();
            }
        }

        m_registeredGameSystemIds.clear();
    }

    bool Engine::EnsureInstanceTransformBufferCapacity(Diligent::Uint32 instanceCount)
    {
        if (instanceCount == 0)
        {
            return false;
        }

        if (m_instanceTransformBuffer && m_instanceTransformCapacity >= instanceCount)
        {
            return true;
        }

        auto* device = m_graphicsRenderer.m_gfx.m_device.RawPtr();
        if (!device)
        {
            return false;
        }

        Uint32 newCapacity = m_instanceTransformCapacity > 0 ? m_instanceTransformCapacity : 1;
        while (newCapacity < instanceCount)
        {
            newCapacity *= 2;
        }

        BufferDesc bufferDesc;
        bufferDesc.Name = "Engine.InstanceTransforms";
        bufferDesc.Usage = USAGE_DYNAMIC;
        bufferDesc.BindFlags = BIND_SHADER_RESOURCE;
        bufferDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        bufferDesc.Size = sizeof(InstanceTransformData) * newCapacity;
        bufferDesc.Mode = BUFFER_MODE_STRUCTURED;
        bufferDesc.ElementByteStride = sizeof(InstanceTransformData);

        RefCntAutoPtr<IBuffer> buffer;
        device->CreateBuffer(bufferDesc, nullptr, &buffer);
        if (!buffer)
        {
            return false;
        }

        m_instanceTransformBuffer = std::move(buffer);
        m_instanceTransformCapacity = newCapacity;
        return true;
    }
} // namespace NexusEngine
