#pragma once

#include <DiligentCore/Common/interface/BasicMath.hpp>

namespace NexusEngine
{
    // Transform component for positioning/rotating/scaling entities
    struct TransformComponent
    {
        Diligent::float3 position = Diligent::float3(0.0f, 0.0f, 0.0f);
        Diligent::float3 rotation = Diligent::float3(0.0f, 0.0f, 0.0f); // Euler angles (radians)
        Diligent::float3 scale = Diligent::float3(1.0f, 1.0f, 1.0f);

        // Get world matrix
        Diligent::float4x4 GetWorldMatrix() const
        {
            auto T = Diligent::float4x4::Translation(position.x, position.y, position.z);
            auto Rx = Diligent::float4x4::RotationX(rotation.x);
            auto Ry = Diligent::float4x4::RotationY(rotation.y);
            auto Rz = Diligent::float4x4::RotationZ(rotation.z);
            auto S = Diligent::float4x4::Scale(scale.x, scale.y, scale.z);
            
            return S * Rz * Ry * Rx * T;
        }
    };
} // namespace NexusEngine
