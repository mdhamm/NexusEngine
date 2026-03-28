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
    struct Material
    {
        // Shaders
        Diligent::RefCntAutoPtr<Diligent::IShader> vertexShader;
        Diligent::RefCntAutoPtr<Diligent::IShader> pixelShader;
        
        // Pipeline state
        Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipelineState;
        
        // Shader resource binding template (can be cloned per mesh instance)
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> shaderResourceBindingTemplate;
        
        // Textures
        Diligent::RefCntAutoPtr<Diligent::ITextureView> albedoTexture;
        Diligent::RefCntAutoPtr<Diligent::ITextureView> normalTexture;
        Diligent::RefCntAutoPtr<Diligent::ITextureView> metallicRoughnessTexture;
        
        // Uniform buffers
        Diligent::RefCntAutoPtr<Diligent::IBuffer> materialConstantBuffer;
        
        // Material properties
        std::string name;
        bool isTransparent = false;
        
        // Render state
        Diligent::CULL_MODE cullMode = Diligent::CULL_MODE_NONE;
        bool depthTestEnabled = true;
        bool depthWriteEnabled = true;
    };
} // namespace NexusEngine
