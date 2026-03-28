#pragma once

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Graphics/GraphicsEngine/interface/ShaderResourceBinding.h>

namespace NexusEngine
{
    struct Mesh;
    struct Material;

    // RenderMeshComponent combines geometry (Mesh) with material for rendering
    // This IS an actual ECS component that gets attached to entities
    struct RenderMeshComponent
    {
        // Reference to mesh geometry (can be shared across multiple entities)
        Mesh* mesh = nullptr;

        // Reference to material (can be shared across multiple entities)
        Material* material = nullptr;

        // Per-instance shader resource binding (if needed for per-object uniforms)
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> instanceSRB;

        // Per-instance constant buffer (for transforms, etc.)
        Diligent::RefCntAutoPtr<Diligent::IBuffer> instanceConstantBuffer;

        // Rendering flags
        bool visible = true;
        bool castShadows = true;
        bool receiveShadows = true;
        int renderLayer = 0; // For sorting/filtering
    };
} // namespace NexusEngine
