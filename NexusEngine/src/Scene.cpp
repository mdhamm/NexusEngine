#include "Scene.h"
#include "ComponentReflection.h"
#include "components/CameraComponent.h"
#include "components/FlyCameraComponent.h"
#include "components/RenderMeshComponent.h"
#include "components/TransformComponent.h"
#include "rendering/Mesh.h"
#include "rendering/Material.h"
#include "rendering/RenderResourceFactory.h"

#include <DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h>
#include <DiligentCore/Graphics/GraphicsTools/interface/MapHelper.hpp>
#include <DiligentCore/Common/interface/BasicMath.hpp>
#include <SDL.h>
#include <cmath>
#include <cstring>
#include <unordered_map>
#include <NexusEngine.cpp>

using namespace Diligent;
namespace NexusEngine
{
    namespace
    {
        void RegisterBuiltinComponentDescriptors()
        {
            static bool s_registered = false;
            if (s_registered)
            {
                return;
            }

            auto& registry = ComponentReflectionRegistry::Instance();
            registry.RegisterDescriptor(TransformComponent::CreateDescriptor());
            registry.RegisterDescriptor(CameraComponent::CreateDescriptor());
            registry.RegisterDescriptor(RenderMeshComponent::CreateDescriptor());
            registry.RegisterDescriptor(FlyCameraComponent::CreateDescriptor());

            s_registered = true;
        }

        // Store matrix as 4 columns (float4 each) to avoid WGSL row-major matrix limitation
        struct InstanceTransformData
        {
            Diligent::float4 m_col0;
            Diligent::float4 m_col1;
            Diligent::float4 m_col2;
            Diligent::float4 m_col3;
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

        Diligent::float4x4 CreateRightHandedProjectionMatrix(float fov, float aspectRatio, float zNear, float zFar)
        {
            const float yScale = 1.0f / std::tan(fov * 0.5f);
            const float xScale = yScale / aspectRatio;

            Diligent::float4x4 projection = Diligent::float4x4::Identity();
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
            // Get the world matrix and extract columns for column-major storage
            // The shader uses float4x4(col0, col1, col2, col3) which constructs column-major
            // So we store the matrix columns directly
            const Diligent::float4x4 world = transform ? transform->GetWorldMatrix() : Diligent::float4x4::Identity();

            InstanceTransformData instanceData{};
            // Extract columns from the row-major Diligent matrix
            // Column 0 = elements [0][0], [1][0], [2][0], [3][0]
            instanceData.m_col0 = Diligent::float4(world.m[0][0], world.m[1][0], world.m[2][0], world.m[3][0]);
            instanceData.m_col1 = Diligent::float4(world.m[0][1], world.m[1][1], world.m[2][1], world.m[3][1]);
            instanceData.m_col2 = Diligent::float4(world.m[0][2], world.m[1][2], world.m[2][2], world.m[3][2]);
            instanceData.m_col3 = Diligent::float4(world.m[0][3], world.m[1][3], world.m[2][3], world.m[3][3]);
            return instanceData;
        }
    }

    Scene::Scene(GraphicsRenderer& graphicsRenderer, Engine& engine, const std::string& name)
        : m_name(name)
        , m_engine(&engine)
        , m_graphicsRenderer(&graphicsRenderer)
    {
        RegisterSceneComponents();
        RegisterPhases();
        RegisterSystems();

        // Initialize resource factory for creating meshes and materials
        if (m_graphicsRenderer->m_gfx.m_device && m_graphicsRenderer->m_gfx.m_ctx)
        {
            m_resourceFactory = std::make_unique<RenderResourceFactory>(
                m_graphicsRenderer->m_gfx.m_device, 
                m_graphicsRenderer->m_gfx.m_ctx);
        }
    }

    Scene::~Scene()
    {
        // Cleanup happens automatically with unique_ptr
    }

    void Scene::Update(float dt)
    {
        m_world.progress(dt);
    }

    void Scene::RegisterSceneComponents()
    {
        RegisterBuiltinComponentDescriptors();
        m_world.component<CameraComponent>();
        m_world.component<RenderTextureComponent>();
        m_world.component<RenderMeshComponent>();
        m_world.component<TransformComponent>();
    }

    void Scene::RegisterPhases()
    {
        // Gameplay logic (abilities, AI, etc.)
        m_world.component<GameplayPhase>()
            .add(flecs::Phase)
            .depends_on(flecs::PreUpdate);

        // Physics updates (movement resolution, collisions)
        m_world.component<PhysicsPhase>()
            .add(flecs::Phase)
            .depends_on<GameplayPhase>();

        // Transform propagation (parent → child)
        m_world.component<TransformPhase>()
            .add(flecs::Phase)
            .depends_on<PhysicsPhase>();

        // Animation evaluation (bones, state machines)
        m_world.component<AnimationPhase>()
            .add(flecs::Phase)
            .depends_on<TransformPhase>();

        // Visibility / culling (frustum, distance, etc.)
        m_world.component<VisibilityPhase>()
            .add(flecs::Phase)
            .depends_on<AnimationPhase>();

        // Prepare rendering work for the frame
        m_world.component<RenderPrepPhase>()
            .add(flecs::Phase)
            .depends_on<VisibilityPhase>();

        // Draw visible scene data
        m_world.component<RenderPhase>()
            .add(flecs::Phase)
            .depends_on<RenderPrepPhase>();

        // Present the completed frame
        m_world.component<RenderPostPhase>()
            .add(flecs::Phase)
            .depends_on<RenderPhase>();
    }

    void Scene::RegisterSystems()
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

                    m_graphicsRenderer->BeginFrame(r, g, b, 1.0f);
                });

        m_world.system<TransformComponent, FlyCameraComponent, CameraComponent>("UpdateFlyCamera")
            .kind<GameplayPhase>()
            .iter(
                [engine = m_engine](flecs::iter& it, TransformComponent* transforms, FlyCameraComponent* controllers, CameraComponent*)
                {
                    const float dt = static_cast<float>(it.delta_time());
                    const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);
                    int mouseDeltaX = 0;
                    int mouseDeltaY = 0;

#if defined(__EMSCRIPTEN__)
                    // Use accumulated mouse movement on web
                    mouseDeltaX = s_accumulatedMouseX;
                    mouseDeltaY = s_accumulatedMouseY;
                    s_accumulatedMouseX = 0;
                    s_accumulatedMouseY = 0;
#else
                    SDL_GetRelativeMouseState(&mouseDeltaX, &mouseDeltaY);
#endif

                    for (auto i : it)
                    {
                        flecs::entity entity = it.entity(i);

                        auto& transform = transforms[i];
                        auto& controller = controllers[i];

                        if (!controller.m_isRotationInitialized)
                        {
                            const Diligent::float3 initialEuler = Quaternion::ToEuler(transform.GetWorldRotation());
                            controller.m_pitch = initialEuler.x;
                            controller.m_yaw = initialEuler.y;
                            controller.m_isRotationInitialized = true;
                        }

                        const float deltaYaw = -static_cast<float>(mouseDeltaX) * controller.m_lookSensitivity;
                        const float deltaPitch = -static_cast<float>(mouseDeltaY) * controller.m_lookSensitivity;
                        constexpr float MaxPitch = Diligent::PI_F * 0.5f - 0.01f;

                        controller.m_yaw += deltaYaw;
                        controller.m_pitch = std::clamp(controller.m_pitch + deltaPitch, -MaxPitch, MaxPitch);

                        Quaternion worldRotation = Quaternion::FromEuler(0.0f, controller.m_yaw, 0.0f);
                        worldRotation = Quaternion::Multiply(worldRotation, Quaternion::FromEuler(controller.m_pitch, 0.0f, 0.0f));

                        const Diligent::float3 forward = Quaternion::Foward(worldRotation);
                        const Diligent::float3 right = Quaternion::Right(worldRotation);

                        SetWorldRotation(entity, worldRotation);

                        const NexusEngine::InputState& input = engine->GetInputState();
                        Diligent::float3 movement(0.0f, 0.0f, 0.0f);

                        if (input.IsKeyDown(NexusEngine::InputKey::W))
                        {
                            movement += forward;
                        }

                        if (input.IsKeyDown(NexusEngine::InputKey::S))
                        {
                            movement -= forward;
                        }

                        if (input.IsKeyDown(NexusEngine::InputKey::D))
                        {
                            movement += right;
                        }

                        if (input.IsKeyDown(NexusEngine::InputKey::A))
                        {
                            movement -= right;
                        }

                        if (Diligent::length(movement) > 0.0f)
                        {
                            movement = Diligent::normalize(movement) * (controller.m_moveSpeed * dt);
                            SetWorldPosition(entity, transform.GetWorldPosition() + movement);
                        }
                    }
                });

        m_world.system<CameraComponent>("RenderScene")
            .kind<RenderPhase>()
            .each(
                [this](flecs::entity cameraEntity, CameraComponent& cam)
                {
                    auto& ctx = m_graphicsRenderer->m_gfx.m_ctx;
                    auto& swapchain = m_graphicsRenderer->m_gfx.m_swapchain;
                    if (!ctx || !swapchain)
                    {
                        return;
                    }

                    const auto& scDesc = swapchain->GetDesc();
                    const float aspect = static_cast<float>(scDesc.Width) / static_cast<float>(scDesc.Height);
                    const auto rtvFormat = scDesc.ColorBufferFormat;
                    const auto dsvFormat = scDesc.DepthBufferFormat;

                    if (cam.m_target == CameraComponent::Target::SwapChain)
                    {
                        Diligent::float4x4 viewMatrix = Diligent::float4x4::Translation(0.0f, 0.0f, -5.0f);
                        if (const auto* cameraTransform = cameraEntity.get<TransformComponent>())
                        {
                            viewMatrix = cameraTransform->GetWorldMatrix().Inverse();
                        }

                        Diligent::float4x4 projMatrix = CreateRightHandedProjectionMatrix(
                            Diligent::PI_F / 4.0f,
                            aspect,
                            0.1f,
                            1000.0f);
                        Diligent::float4x4 viewProjMatrix = viewMatrix * projMatrix;

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
                                    singleDraws.push_back(SingleDraw{
                                        renderMesh.mesh,
                                        renderMesh.material,
                                        renderMesh.instanceSRB.RawPtr(),
                                        instanceData });
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
                                    // Store ViewProj as 4 columns to avoid WGSL row-major matrix issues
                                    struct VSConstantsData
                                    {
                                        Diligent::float4 col0;
                                        Diligent::float4 col1;
                                        Diligent::float4 col2;
                                        Diligent::float4 col3;
                                    };

                                    MapHelper<VSConstantsData> mappedData(
                                        ctx,
                                        material->materialConstantBuffer,
                                        MAP_WRITE,
                                        MAP_FLAG_DISCARD);

                                    // Extract columns from the row-major Diligent matrix
                                    mappedData->col0 = Diligent::float4(viewProjMatrix.m[0][0], viewProjMatrix.m[1][0], viewProjMatrix.m[2][0], viewProjMatrix.m[3][0]);
                                    mappedData->col1 = Diligent::float4(viewProjMatrix.m[0][1], viewProjMatrix.m[1][1], viewProjMatrix.m[2][1], viewProjMatrix.m[3][1]);
                                    mappedData->col2 = Diligent::float4(viewProjMatrix.m[0][2], viewProjMatrix.m[1][2], viewProjMatrix.m[2][2], viewProjMatrix.m[3][2]);
                                    mappedData->col3 = Diligent::float4(viewProjMatrix.m[0][3], viewProjMatrix.m[1][3], viewProjMatrix.m[2][3], viewProjMatrix.m[3][3]);
                                }

                                MapHelper<InstanceTransformData> mappedInstances(
                                    ctx,
                                    m_instanceTransformBuffer,
                                    MAP_WRITE,
                                    MAP_FLAG_DISCARD);

                                // Instance data is already in column format from CreateInstanceTransformData
                                std::memcpy(mappedInstances, instanceData, sizeof(InstanceTransformData) * instanceCount);

                                // Bind the StructuredBuffer for instance data to the shader
                                if (auto* instanceVar = srb->GetVariableByName(SHADER_TYPE_VERTEX, "g_InstanceData"))
                                {
                                    instanceVar->Set(m_instanceTransformBuffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
                                }

                                ctx->SetPipelineState(cachedPipeline->m_pipelineState);
                                ctx->CommitShaderResources(srb, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

                                IBuffer* vertexBuffers[] = { mesh->vertexBuffer };
                                Uint64 offsets[] = { 0 };
                                ctx->SetVertexBuffers(
                                    0,
                                    1,
                                    vertexBuffers,
                                    offsets,
                                    RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                                    SET_VERTEX_BUFFERS_FLAG_RESET);

                                if (mesh->indexBuffer && mesh->indexCount > 0)
                                {
                                    ctx->SetIndexBuffer(
                                        mesh->indexBuffer,
                                        0,
                                        RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

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
                            drawInstances(
                                batchKey.m_mesh,
                                batchKey.m_material,
                                nullptr,
                                batchInstances.data(),
                                static_cast<Uint32>(batchInstances.size()));
                        }

                        for (const SingleDraw& draw : singleDraws)
                        {
                            drawInstances(draw.m_mesh, draw.m_material, draw.m_srb, &draw.m_instanceData, 1);
                        }
                    }
                    else if (cam.m_target == CameraComponent::Target::RenderTexture)
                    {
                        // TODO: Render to texture target
                        // Similar to SwapChain rendering but set render target from cam.m_renderTexture
                    }
                });

        m_world.system<>("EndFrame")
            .kind<RenderPostPhase>()
            .iter(
                [this](flecs::iter&)
                {
                    m_graphicsRenderer->EndFrame();
                });
    }

    bool Scene::EnsureInstanceTransformBufferCapacity(Diligent::Uint32 instanceCount)
    {
        if (instanceCount == 0)
        {
            return false;
        }

        if (m_instanceTransformBuffer && m_instanceTransformCapacity >= instanceCount)
        {
            return true;
        }

        auto* device = m_graphicsRenderer->m_gfx.m_device.RawPtr();
        if (!device)
        {
            return false;
        }

        Diligent::Uint32 newCapacity = m_instanceTransformCapacity > 0 ? m_instanceTransformCapacity : 1;
        while (newCapacity < instanceCount)
        {
            newCapacity *= 2;
        }

        BufferDesc bufferDesc;
        bufferDesc.Name = "Scene.InstanceTransforms";
        bufferDesc.Usage = USAGE_DYNAMIC;
        bufferDesc.BindFlags = BIND_SHADER_RESOURCE;
        bufferDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        bufferDesc.Size = sizeof(InstanceTransformData) * newCapacity;
        bufferDesc.Mode = BUFFER_MODE_STRUCTURED;
        bufferDesc.ElementByteStride = sizeof(InstanceTransformData);

        Diligent::RefCntAutoPtr<Diligent::IBuffer> buffer;
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

