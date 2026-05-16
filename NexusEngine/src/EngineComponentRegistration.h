#pragma once

#include "GameLoopPhases.h"
#include "components/CameraComponent.h"
#include "components/EditorOnlyComponent.h"
#include "components/FlyCameraComponent.h"
#include "components/RenderMeshComponent.h"
#include "components/RenderTextureComponent.h"
#include "components/TransformComponent.h"

namespace NexusEngine
{
    /// <summary>
    /// Registers engine-owned component and tag types with a Flecs world for the current module.
    /// </summary>
    /// <param name="world">World that should know the engine-owned type ids.</param>
    inline void RegisterEngineComponentTypes(flecs::world& world)
    {
        world.component<TransformComponent>();
        world.component<CameraComponent>();
        world.component<RenderTextureComponent>();
        world.component<RenderMeshComponent>();
        world.component<FlyCameraComponent>();
        world.component<EditorOnlyComponent>();

        world.component<GameplayPhase>();
        world.component<GameplayEnabled>();
        world.component<PhysicsPhase>();
        world.component<PhysicsEnabled>();
        world.component<TransformPhase>();
        world.component<AnimationPhase>();
        world.component<VisibilityPhase>();
        world.component<RenderPrepPhase>();
        world.component<RenderPhase>();
        world.component<RenderPostPhase>();
    }
} // namespace NexusEngine
