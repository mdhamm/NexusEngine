#pragma once

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Graphics/GraphicsEngine/interface/Buffer.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/GraphicsTypes.h>
#include <string>

namespace NexusEngine
{
    // Raw mesh geometry resource shared by renderable entities.
    struct Mesh
    {
        // Vertex buffer containing mesh vertices.
        Diligent::RefCntAutoPtr<Diligent::IBuffer> vertexBuffer;

        // Index buffer containing mesh indices.
        Diligent::RefCntAutoPtr<Diligent::IBuffer> indexBuffer;

        // Number of vertices in the mesh.
        Diligent::Uint32 vertexCount = 0;

        // Number of indices in the mesh.
        Diligent::Uint32 indexCount = 0;

        // Size of a single vertex in bytes.
        Diligent::Uint32 vertexStride = 0;

        // Primitive topology used when drawing the mesh.
        Diligent::PRIMITIVE_TOPOLOGY topology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // Debug or asset name for the mesh.
        std::string name;
    };
} // namespace NexusEngine

