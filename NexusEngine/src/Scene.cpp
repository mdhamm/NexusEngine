#include "Scene.h"
#include <DiligentCore/Common/interface/BasicMath.hpp>
#include "CameraComponent.h"
#include "CanvasComponent.h"
#include "components/RenderProxy.h"

using namespace Diligent;
namespace NexusEngine
{
    void RegisterSceneComponents(flecs::world& w)
    {
        w.component<CameraComponent>();
        w.component<CanvasComponent>();
        w.component<RenderProxy>();
    }

    void Scene::update(float dt)
    {
        world.progress(dt);
    }

    void RenderScene(Scene& scene)
    {
        // Iterate all cameras with an attached render target
        scene.world.each<CameraComponent>(
            [&](flecs::entity e, CameraComponent& cam)
            {
                if (!cam.targetRTV || !cam.targetDSV)
                    return;

                auto* ctx = cam.deviceCtx;
                ITextureView* rtvs[] = { cam.targetRTV.RawPtr<>() };
                ctx->SetRenderTargets(1, rtvs, cam.targetDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
                const float clear[4] = { 0.05f, 0.05f, 0.08f, 1.0f };
                ctx->ClearRenderTarget(rtvs[0], clear, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
                ctx->ClearDepthStencil(cam.targetDSV, CLEAR_DEPTH_FLAG, 1.0f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

                // TODO: draw renderables, meshes, etc.

                // Draw UI overlay (Canvas)
                if (cam.canvas.is_alive())
                {
                    auto canvasComp = cam.canvas.get<CanvasComponent>();
                    if (canvasComp && canvasComp->uiLayerFn)
                        canvasComp->uiLayerFn();
                }
            });
    }
} // namespace NexusEngine
