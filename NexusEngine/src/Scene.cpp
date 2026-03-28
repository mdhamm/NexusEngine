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

#include <functional>

using namespace Diligent;
namespace NexusEngine
{
    Scene::Scene(GraphicsRenderer& graphicsRenderer, const std::string& name)
        : m_name(name)
        , m_graphicsRenderer(graphicsRenderer)
    {
        RegisterSceneComponents();

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

        auto composeWorldTransform = [](TransformComponent& transform, const TransformComponent* parentTransform)
        {
            if (parentTransform)
            {
                transform.m_worldPosition = parentTransform->m_worldPosition + transform.m_localPosition;
                transform.m_worldRotation = parentTransform->m_worldRotation + transform.m_localRotation;
                transform.m_worldScale = Diligent::float3(
                    parentTransform->m_worldScale.x * transform.m_localScale.x,
                    parentTransform->m_worldScale.y * transform.m_localScale.y,
                    parentTransform->m_worldScale.z * transform.m_localScale.z);
                transform.m_worldMatrix = transform.GetLocalMatrix() * parentTransform->GetWorldMatrix();
            }
            else
            {
                transform.m_worldPosition = transform.m_localPosition;
                transform.m_worldRotation = transform.m_localRotation;
                transform.m_worldScale = transform.m_localScale;
                transform.m_worldMatrix = transform.GetLocalMatrix();
            }
        };

        std::function<void(flecs::entity, const TransformComponent*)> propagateHierarchy;
        propagateHierarchy = [&](flecs::entity entity, const TransformComponent* parentTransform)
        {
            const TransformComponent* currentTransform = parentTransform;

            if (auto* transform = entity.get_mut<TransformComponent>())
            {
                composeWorldTransform(*transform, parentTransform);
                currentTransform = transform;
            }

            entity.children(
                [&](flecs::entity child)
                {
                    propagateHierarchy(child, currentTransform);
                });
        };

        m_world.each<TransformComponent>(
            [&](flecs::entity entity, TransformComponent&)
            {
                if (auto parent = entity.parent(); parent)
                {
                    if (!parent.has<TransformComponent>())
                    {
                        propagateHierarchy(entity, nullptr);
                    }
                }
                else
                {
                    propagateHierarchy(entity, nullptr);
                }
            });
    }

    void Scene::Render()
    {
        auto& ctx = m_graphicsRenderer.m_gfx.m_ctx;
        if (!ctx)
            return;

        // Setup camera matrices (TODO: get from actual camera component)
        const auto& scDesc = m_graphicsRenderer.m_gfx.m_swapchain->GetDesc();
        float aspect = static_cast<float>(scDesc.Width) / static_cast<float>(scDesc.Height);

        Diligent::float4x4 viewMatrix = Diligent::float4x4::Translation(0.0f, 0.0f, 5.0f);
        Diligent::float4x4 projMatrix = Diligent::float4x4::Projection(
            Diligent::PI_F / 4.0f,
            aspect,
            0.1f,
            100.0f,
            false);
        Diligent::float4x4 viewProjMatrix = viewMatrix * projMatrix;

        // Iterate all cameras with an attached render target
        m_world.each<CameraComponent>(
            [&](flecs::entity, CameraComponent& cam)
            {
                if (cam.m_target == CameraComponent::Target::SwapChain)
                {
                    // Draw all visible RenderMesh components for this camera
                    m_world.each<RenderMeshComponent>(
                        [&](flecs::entity meshEntity, RenderMeshComponent& renderMesh)
                        {
                            // Validate render mesh
                            if (!renderMesh.visible || !renderMesh.mesh || !renderMesh.material)
                                return;

                            Mesh* mesh = renderMesh.mesh;
                            Material* material = renderMesh.material;

                            // Validate material pipeline
                            if (!material->pipelineState)
                                return;

                            // Get transform (default to identity if not present)
                            Diligent::float4x4 worldMatrix = Diligent::float4x4::Identity();
                            if (auto* transform = meshEntity.get<TransformComponent>())
                            {
                                worldMatrix = transform->GetWorldMatrix();
                            }

                            // Set pipeline state
                            ctx->SetPipelineState(material->pipelineState);

                            // Determine which SRB to use: per-instance or material template
                            IShaderResourceBinding* srb = renderMesh.instanceSRB
                                ? renderMesh.instanceSRB.RawPtr()
                                : material->shaderResourceBindingTemplate.RawPtr();

                            if (!srb)
                                return;

                            // Update constant buffer every frame before first use in this frame.
                            // D3D12 backend may round CB size (e.g. to 256), so do not rely on exact sizes.
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

                            // Commit shader resources
                            ctx->CommitShaderResources(srb, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

                            // Set vertex buffer
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

                            // Draw
                            if (mesh->indexBuffer && mesh->indexCount > 0)
                            {
                                // Indexed draw
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
                                // Non-indexed draw
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
    }

    void Scene::RegisterSceneComponents()
    {
        m_world.component<CameraComponent>();
        m_world.component<RenderTextureComponent>();
        m_world.component<RenderMeshComponent>();
        m_world.component<TransformComponent>();
    }
} // namespace NexusEngine

