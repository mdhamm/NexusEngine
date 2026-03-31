#include "Scene.h"
#include "components/CameraComponent.h"
#include "components/RenderMeshComponent.h"
#include "components/TransformComponent.h"
#include "rendering/Mesh.h"
#include "rendering/Material.h"
#include "rendering/RenderResourceFactory.h"

#include <DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h>
#include <DiligentCore/Graphics/GraphicsTools/interface/MapHelper.hpp>
#include <DiligentCore/Common/interface/BasicMath.hpp>

#include <cmath>

using namespace Diligent;
namespace NexusEngine
{
    namespace
    {
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
    }

    Scene::Scene(GraphicsRenderer& graphicsRenderer, const std::string& name)
        : m_name(name)
        , m_graphicsRenderer(graphicsRenderer)
    {
        RegisterSceneComponents();
        RegisterPhases();
        RegisterSystems();

        // Initialize resource factory for creating meshes and materials
        if (m_graphicsRenderer.m_gfx.m_device && m_graphicsRenderer.m_gfx.m_ctx)
        {
            m_resourceFactory = std::make_unique<RenderResourceFactory>(
                m_graphicsRenderer.m_gfx.m_device, 
                m_graphicsRenderer.m_gfx.m_ctx);
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
        m_world.component<CameraComponent>();
        m_world.component<RenderTextureComponent>();
        m_world.component<RenderMeshComponent>();
        m_world.component<TransformComponent>();
    }

    void Scene::RegisterPhases()
    {
        // Input happens before gameplay
        m_world.component<InputPhase>()
            .add(flecs::Phase)
            .depends_on(flecs::PreUpdate);

        // Gameplay logic (abilities, AI, etc.)
        m_world.component<GameplayPhase>()
            .add(flecs::Phase)
            .depends_on<InputPhase>();

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

                    m_graphicsRenderer.BeginFrame(r, g, b, 1.0f);
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

                        m_world.each<RenderMeshComponent>(
                            [&](flecs::entity meshEntity, RenderMeshComponent& renderMesh)
                            {
                                if (!renderMesh.visible || !renderMesh.mesh || !renderMesh.material)
                                {
                                    return;
                                }

                                Mesh* mesh = renderMesh.mesh;
                                Material* material = renderMesh.material;

                                auto* cachedPipeline = m_resourceFactory
                                    ? m_resourceFactory->GetOrCreatePipeline(material, mesh, rtvFormat, dsvFormat)
                                    : nullptr;
                                if (!cachedPipeline || !cachedPipeline->m_pipelineState)
                                {
                                    return;
                                }

                                Diligent::float4x4 worldMatrix = Diligent::float4x4::Identity();
                                if (auto* transform = meshEntity.get<TransformComponent>())
                                {
                                    worldMatrix = transform->GetWorldMatrix();
                                }

                                ctx->SetPipelineState(cachedPipeline->m_pipelineState);

                                IShaderResourceBinding* srb = renderMesh.instanceSRB
                                    ? renderMesh.instanceSRB.RawPtr()
                                    : cachedPipeline->m_shaderResourceBindingTemplate.RawPtr();
                                if (!srb)
                                {
                                    return;
                                }

                                if (material->materialConstantBuffer)
                                {
                                    struct VSConstants
                                    {
                                        Diligent::float4x4 WorldViewProj;
                                        Diligent::float4x4 World;
                                    };

                                    const auto& bufDesc = material->materialConstantBuffer->GetDesc();
                                    const Diligent::float4x4 worldViewProj = worldMatrix * viewProjMatrix;
                                    const Diligent::float4x4 worldViewProjT = worldViewProj.Transpose();
                                    const Diligent::float4x4 worldMatrixT = worldMatrix.Transpose();

                                    if (bufDesc.Size >= sizeof(VSConstants))
                                    {
                                        VSConstants constants{};
                                        constants.World = worldMatrixT;
                                        constants.WorldViewProj = worldViewProjT;

                                        MapHelper<VSConstants> mappedData(
                                            ctx,
                                            material->materialConstantBuffer,
                                            MAP_WRITE,
                                            MAP_FLAG_DISCARD);
                                        *mappedData = constants;
                                    }
                                    else
                                    {
                                        MapHelper<Diligent::float4x4> mappedData(
                                            ctx,
                                            material->materialConstantBuffer,
                                            MAP_WRITE,
                                            MAP_FLAG_DISCARD);
                                        *mappedData = worldViewProjT;
                                    }
                                }

                                ctx->CommitShaderResources(srb, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

                                if (mesh->vertexBuffer)
                                {
                                    IBuffer* vertexBuffers[] = { mesh->vertexBuffer };
                                    Uint64 offsets[] = { 0 };
                                    ctx->SetVertexBuffers(
                                        0, 1,
                                        vertexBuffers,
                                        offsets,
                                        RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                                        SET_VERTEX_BUFFERS_FLAG_RESET);
                                }

                                if (mesh->indexBuffer && mesh->indexCount > 0)
                                {
                                    ctx->SetIndexBuffer(
                                        mesh->indexBuffer,
                                        0,
                                        RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

                                    DrawIndexedAttribs drawAttrs;
                                    drawAttrs.IndexType = VT_UINT32;
                                    drawAttrs.NumIndices = mesh->indexCount;
                                    drawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;

                                    ctx->DrawIndexed(drawAttrs);
                                }
                                else if (mesh->vertexCount > 0)
                                {
                                    DrawAttribs drawAttrs;
                                    drawAttrs.NumVertices = mesh->vertexCount;
                                    drawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;

                                    ctx->Draw(drawAttrs);
                                }
                            });
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
                    m_graphicsRenderer.EndFrame();
                });
    }
} // namespace NexusEngine

