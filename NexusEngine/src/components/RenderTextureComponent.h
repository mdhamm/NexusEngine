#pragma once

#include <DiligentCore/Graphics/GraphicsEngine/interface/TextureView.h>

namespace NexusEngine
{
    struct RenderTextureComponent
    {
        Diligent::RefCntAutoPtr<Diligent::ITexture>     m_texture;
        Diligent::RefCntAutoPtr<Diligent::ITextureView> m_rtv;
        Diligent::RefCntAutoPtr<Diligent::ITextureView> m_srv;
        int m_width = 256;
        int m_height = 256;
    };
} // namespace NexusEngine