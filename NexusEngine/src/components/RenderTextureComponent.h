#pragma once

#include "reflection/EntityReflection.h"

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Graphics/GraphicsEngine/interface/TextureView.h>

namespace NexusEngine
{
    // Off-screen render target owned by an entity or camera.
    struct RenderTextureComponent
    {
        // Backing texture resource.
        Diligent::RefCntAutoPtr<Diligent::ITexture>     m_texture;

        // Render-target view for writing into the texture.
        Diligent::RefCntAutoPtr<Diligent::ITextureView> m_rtv;

        // Shader-resource view for sampling the texture.
        Diligent::RefCntAutoPtr<Diligent::ITextureView> m_srv;

        // Texture width in pixels.
        int m_width = 256;

        // Texture height in pixels.
        int m_height = 256;
    };

    template<>
    struct ComponentMeta<RenderTextureComponent>
    {
        static void Register(flecs::world& world, MetadataRegistry& registry)
        {
            world.component<RenderTextureComponent>()
                .member<int>("width")
                .member<int>("height");

            registry.component<RenderTextureComponent>("RenderTextureComponent")
                .field("Width", &RenderTextureComponent::m_width)
                .field("Height", &RenderTextureComponent::m_height);
        }
    };
} // namespace NexusEngine