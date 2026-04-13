#pragma once

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Graphics/GraphicsEngine/interface/Shader.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/Texture.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/Buffer.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/PipelineState.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/ShaderResourceBinding.h>
#include <string>
#include <vector>

namespace NexusEngine
{
    // Material description containing shaders, state, and shared bindings.
    struct Material
    {
        // Vertex shader used by the material.
        Diligent::RefCntAutoPtr<Diligent::IShader> vertexShader;

        // Pixel shader used by the material.
        Diligent::RefCntAutoPtr<Diligent::IShader> pixelShader;
        
        // Pipeline state object used for rendering.
        Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipelineState;
        
        // Shared shader resource binding template for per-instance clones.
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> shaderResourceBindingTemplate;
        
        // Base color texture view.
        Diligent::RefCntAutoPtr<Diligent::ITextureView> albedoTexture;

        // Normal map texture view.
        Diligent::RefCntAutoPtr<Diligent::ITextureView> normalTexture;

        // Metallic-roughness texture view.
        Diligent::RefCntAutoPtr<Diligent::ITextureView> metallicRoughnessTexture;
        
        // Constant buffer holding material parameters.
        Diligent::RefCntAutoPtr<Diligent::IBuffer> materialConstantBuffer;

        // Input layout used to build compatible pipeline variants.
        std::vector<Diligent::LayoutElement> m_inputLayout;
        
        // Debug or asset name for the material.
        std::string name;

        // Enables alpha blending style rendering paths.
        bool isTransparent = false;
        
        // Face culling mode.
        Diligent::CULL_MODE cullMode = Diligent::CULL_MODE_NONE;

        // Enables depth testing.
        bool depthTestEnabled = true;

        // Enables depth writes.
        bool depthWriteEnabled = true;
    };
} // namespace NexusEngine
