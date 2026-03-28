#pragma once

#include <DiligentCore/Common/interface/BasicMath.hpp>

namespace NexusEngine
{
    // Transform component for positioning/rotating/scaling entities
    struct TransformComponent
    {
        Diligent::float3 m_localPosition = Diligent::float3(0.0f, 0.0f, 0.0f);
        Diligent::float3 m_localRotation = Diligent::float3(0.0f, 0.0f, 0.0f);
        Diligent::float3 m_localScale = Diligent::float3(1.0f, 1.0f, 1.0f);

        Diligent::float3 m_worldPosition = Diligent::float3(0.0f, 0.0f, 0.0f);
        Diligent::float3 m_worldRotation = Diligent::float3(0.0f, 0.0f, 0.0f);
        Diligent::float3 m_worldScale = Diligent::float3(1.0f, 1.0f, 1.0f);
        Diligent::float4x4 m_worldMatrix = Diligent::float4x4::Identity();

        Diligent::float4x4 GetLocalMatrix() const
        {
            auto T = Diligent::float4x4::Translation(m_localPosition.x, m_localPosition.y, m_localPosition.z);
            auto Rx = Diligent::float4x4::RotationX(m_localRotation.x);
            auto Ry = Diligent::float4x4::RotationY(m_localRotation.y);
            auto Rz = Diligent::float4x4::RotationZ(m_localRotation.z);
            auto S = Diligent::float4x4::Scale(m_localScale.x, m_localScale.y, m_localScale.z);
            
            return S * Rz * Ry * Rx * T;
        }

        Diligent::float4x4 GetWorldMatrix() const
        {
            return m_worldMatrix;
        }
    };
} // namespace NexusEngine
