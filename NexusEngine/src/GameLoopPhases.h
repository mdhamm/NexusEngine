#pragma once

namespace NexusEngine
{
    // Marker for the gameplay phase in the scene pipeline.
    struct GameplayPhase {};
    struct GameplayEnabled {};

    // Marker for the physics phase in the scene pipeline.
    struct PhysicsPhase {};
    struct PhysicsEnabled {};

    // Marker for the transform phase in the scene pipeline.
    struct TransformPhase {};

    // Marker for the animation phase in the scene pipeline.
    struct AnimationPhase {};

    // Marker for the visibility phase in the scene pipeline.
    struct VisibilityPhase {};

    // Marker for the render preparation phase in the scene pipeline.
    struct RenderPrepPhase {};

    // Marker for the main render phase in the scene pipeline.
    struct RenderPhase {};

    // Marker for the post-render phase in the scene pipeline.
    struct RenderPostPhase {};
} // namespace NexusEngine