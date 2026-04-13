#pragma once

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Graphics/GraphicsEngine/interface/ShaderResourceBinding.h>

namespace NexusEngine
{
    struct Mesh;
    struct Material;

    // ECS component that binds a mesh and material to an entity for rendering.
    struct RenderMeshComponent
    {
        // Shared mesh geometry used by this entity.
        Mesh* mesh = nullptr;

        // Shared material used to render the mesh.
        Material* material = nullptr;

        // Optional per-instance shader resource binding.
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> instanceSRB;

        // Optional per-instance constant buffer.
        Diligent::RefCntAutoPtr<Diligent::IBuffer> instanceConstantBuffer;

        // Enables or disables rendering for this component.
        bool visible = true;

        // Enables shadow casting for this mesh.
        bool castShadows = true;

        // Enables receiving shadows for this mesh.
        bool receiveShadows = true;

        // Render layer used for filtering and ordering.
        int renderLayer = 0; // For sorting/filtering
    };
} // namespace NexusEngine
