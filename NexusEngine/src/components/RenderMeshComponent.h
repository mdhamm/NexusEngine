#pragma once

#include "reflection/EntityReflection.h"

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Graphics/GraphicsEngine/interface/Buffer.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/ShaderResourceBinding.h>

namespace NexusEngine
{
    struct Mesh;
    struct Material;
    struct RenderMeshComponent;

    // ECS component that binds a mesh and material to an entity for rendering.
    struct RenderMeshComponent
    {
        // Shared mesh geometry used by this entity.
        Mesh* mesh = nullptr;

        // Shared material used to render the mesh.
        Material* material = nullptr;

        // Material asset path used to resolve the runtime material instance.
        std::string m_materialAssetPath;

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

    template<>
    struct ComponentMeta<RenderMeshComponent>
    {
        static void Register(flecs::world& world, MetadataRegistry& registry)
        {
            world.component<RenderMeshComponent>()
                .member<std::string>("materialAssetPath")
                .member<bool>("visible")
                .member<bool>("castShadows")
                .member<bool>("receiveShadows")
                .member<int>("renderLayer");

            registry.component<RenderMeshComponent>("RenderMeshComponent")
                .field("Material Asset", &RenderMeshComponent::m_materialAssetPath)
                    .assetType(AssetType::Material)
                .field("Visible", &RenderMeshComponent::visible)
                .field("Cast Shadows", &RenderMeshComponent::castShadows)
                .field("Receive Shadows", &RenderMeshComponent::receiveShadows)
                .field("Render Layer", &RenderMeshComponent::renderLayer);
        }
    };
} // namespace NexusEngine
