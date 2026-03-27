#pragma once

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Graphics/GraphicsEngine/interface/Buffer.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/PipelineState.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/ShaderResourceBinding.h>
#include <string>

namespace NexusEngine
{
    struct MeshComponent
    {
        // Geometry data
        Diligent::RefCntAutoPtr<Diligent::IBuffer> vertexBuffer;
        Diligent::RefCntAutoPtr<Diligent::IBuffer> indexBuffer;

        Diligent::Uint32 vertexCount = 0;
        Diligent::Uint32 indexCount = 0;
        Diligent::Uint32 vertexStride = 0;

        // Rendering resources
        Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipelineState;
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> shaderResourceBinding;

        // Material/Mesh identifier (optional)
        std::string meshName;

        // Rendering state
        bool visible = true;
        Diligent::PRIMITIVE_TOPOLOGY topology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    };
} // namespace NexusEngine
