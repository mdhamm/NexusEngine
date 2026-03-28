#pragma once

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Graphics/GraphicsEngine/interface/Buffer.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/GraphicsTypes.h>
#include <string>

namespace NexusEngine
{
    // Mesh contains only raw geometry data (not an ECS component itself)
    struct Mesh
    {
        // Geometry buffers
        Diligent::RefCntAutoPtr<Diligent::IBuffer> vertexBuffer;
        Diligent::RefCntAutoPtr<Diligent::IBuffer> indexBuffer;

        // Geometry metadata
        Diligent::Uint32 vertexCount = 0;
        Diligent::Uint32 indexCount = 0;
        Diligent::Uint32 vertexStride = 0;

        // Topology
        Diligent::PRIMITIVE_TOPOLOGY topology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // Mesh identifier
        std::string name;
    };
} // namespace NexusEngine

