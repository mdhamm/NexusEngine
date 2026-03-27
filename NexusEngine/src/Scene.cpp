#include "Scene.h"
#include "components/CameraComponent.h"
#include "components/MeshComponent.h"

using namespace Diligent;
namespace NexusEngine
{
    Scene::Scene(GraphicsRenderer& graphicsRenderer, const std::string& name)
        : m_name(name)
        , m_graphicsRenderer(graphicsRenderer)
    {
        RegisterSceneComponents();
    }

    void Scene::Update(float dt)
    {
        m_world.progress(dt);
    }

    void Scene::Render()
    {
        auto& ctx = m_graphicsRenderer.m_gfx.m_ctx;
        if (!ctx)
            return;

        // Iterate all cameras with an attached render target
        m_world.each<CameraComponent>(
            [&](flecs::entity e, CameraComponent& cam)
            {
                if (cam.m_target == CameraComponent::Target::SwapChain)
                {
                    // Draw all visible meshes for this camera
                    m_world.each<MeshComponent>(
                        [&](flecs::entity meshEntity, MeshComponent& mesh)
                        {
                            if (!mesh.visible || !mesh.pipelineState)
                                return;

                            // Set pipeline state
                            ctx->SetPipelineState(mesh.pipelineState);

                            // Bind shader resources if available
                            if (mesh.shaderResourceBinding)
                            {
                                ctx->CommitShaderResources(mesh.shaderResourceBinding, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
                            }

                            // Set vertex buffer
                            if (mesh.vertexBuffer)
                            {
                                IBuffer* vertexBuffers[] = { mesh.vertexBuffer };
                                Uint64 offsets[] = { 0 };
                                ctx->SetVertexBuffers(0, 1, vertexBuffers, offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
                            }

                            // Draw
                            if (mesh.indexBuffer && mesh.indexCount > 0)
                            {
                                // Indexed draw
                                ctx->SetIndexBuffer(mesh.indexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

                                DrawIndexedAttribs drawAttrs;
                                drawAttrs.IndexType = VT_UINT32; // Assuming 32-bit indices
                                drawAttrs.NumIndices = mesh.indexCount;
                                drawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;

                                ctx->DrawIndexed(drawAttrs);
                            }
                            else if (mesh.vertexCount > 0)
                            {
                                // Non-indexed draw
                                DrawAttribs drawAttrs;
                                drawAttrs.NumVertices = mesh.vertexCount;
                                drawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;

                                ctx->Draw(drawAttrs);
                            }
                        });
                }
                else if (cam.m_target == CameraComponent::Target::RenderTexture)
                {
                    // TODO: Render to texture target
                }
            });
    }

    void Scene::RegisterSceneComponents()
    {
        m_world.component<CameraComponent>();
        m_world.component<MeshComponent>();
        m_world.component<RenderTextureComponent>();
    }
} // namespace NexusEngine
