#pragma once

#include <cassert>
#include <cmath>

#include <flecs.h>
#include <DiligentCore/Common/interface/BasicMath.hpp>

namespace NexusEngine
{
    // Minimal quaternion helper used by transform code.
    struct Quaternion
    {
        // X component of the quaternion vector part.
        float x = 0.0f;

        // Y component of the quaternion vector part.
        float y = 0.0f;

        // Z component of the quaternion vector part.
        float z = 0.0f;

        // W component of the quaternion scalar part.
        float w = 1.0f;

        /// <summary>
        /// Returns the identity rotation.
        /// </summary>
        /// <returns>An identity quaternion.</returns>
        static Quaternion Identity()
        {
            return Quaternion{};
        }

        /// <summary>
        /// Returns a normalized quaternion.
        /// </summary>
        /// <param name="q">Quaternion to normalize.</param>
        /// <returns>The normalized quaternion.</returns>
        static Quaternion Normalize(const Quaternion& q)
        {
            const float lenSq = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
            if (lenSq <= 0.0f)
            {
                return Identity();
            }

            const float invLen = 1.0f / std::sqrt(lenSq);

            Quaternion result;
            result.x = q.x * invLen;
            result.y = q.y * invLen;
            result.z = q.z * invLen;
            result.w = q.w * invLen;
            return result;
        }

        /// <summary>
        /// Returns the quaternion conjugate.
        /// </summary>
        /// <param name="q">Quaternion to conjugate.</param>
        /// <returns>The conjugated quaternion.</returns>
        static Quaternion Conjugate(const Quaternion& q)
        {
            Quaternion result;
            result.x = -q.x;
            result.y = -q.y;
            result.z = -q.z;
            result.w = q.w;
            return result;
        }

        /// <summary>
        /// Multiplies two quaternions.
        /// </summary>
        /// <param name="a">Left-hand quaternion.</param>
        /// <param name="b">Right-hand quaternion.</param>
        /// <returns>The multiplied quaternion.</returns>
        static Quaternion Multiply(const Quaternion& a, const Quaternion& b)
        {
            Quaternion result;
            result.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
            result.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
            result.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
            result.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
            return result;
        }

        /// <summary>
        /// Builds a quaternion from an axis-angle rotation.
        /// </summary>
        /// <param name="axis">Rotation axis.</param>
        /// <param name="angleRadians">Rotation angle in radians.</param>
        /// <returns>The resulting quaternion.</returns>
        static Quaternion FromAxisAngle(const Diligent::float3& axis, float angleRadians)
        {
            const float halfAngle = angleRadians * 0.5f;
            const float s = std::sin(halfAngle);

            Quaternion result;
            result.x = axis.x * s;
            result.y = axis.y * s;
            result.z = axis.z * s;
            result.w = std::cos(halfAngle);
            return Normalize(result);
        }

        /// <summary>
        /// Builds a quaternion from Euler angles in radians.
        /// </summary>
        /// <param name="pitchRadians">Pitch rotation in radians.</param>
        /// <param name="yawRadians">Yaw rotation in radians.</param>
        /// <param name="rollRadians">Roll rotation in radians.</param>
        /// <returns>The resulting quaternion.</returns>
        static Quaternion FromEuler(float pitchRadians, float yawRadians, float rollRadians)
        {
            const Quaternion qx = FromAxisAngle(Diligent::float3(1.0f, 0.0f, 0.0f), pitchRadians);
            const Quaternion qy = FromAxisAngle(Diligent::float3(0.0f, 1.0f, 0.0f), yawRadians);
            const Quaternion qz = FromAxisAngle(Diligent::float3(0.0f, 0.0f, 1.0f), rollRadians);

            return Multiply(Multiply(qx, qy), qz);
        }

        /// <summary>
        /// Builds a quaternion from an Euler vector in radians.
        /// </summary>
        /// <param name="eulerRadians">Euler rotation vector in radians.</param>
        /// <returns>The resulting quaternion.</returns>
        static Quaternion FromEuler(const Diligent::float3& eulerRadians)
        {
            return FromEuler(eulerRadians.x, eulerRadians.y, eulerRadians.z);
        }

        /// <summary>
        /// Converts a quaternion to Euler angles in radians.
        /// </summary>
        /// <param name="q">Quaternion to convert.</param>
        /// <returns>Euler rotation in radians.</returns>
        static Diligent::float3 ToEuler(const Quaternion& q)
        {
            const Quaternion n = Normalize(q);

            const float xx = n.x * n.x;
            const float yy = n.y * n.y;
            const float zz = n.z * n.z;
            const float xy = n.x * n.y;
            const float xz = n.x * n.z;
            const float yz = n.y * n.z;
            const float wx = n.w * n.x;
            const float wy = n.w * n.y;
            const float wz = n.w * n.z;

            // Rotation matrix from quaternion
            const float m00 = 1.0f - 2.0f * (yy + zz);
            const float m01 = 2.0f * (xy - wz);
            const float m02 = 2.0f * (xz + wy);

            const float m10 = 2.0f * (xy + wz);
            const float m11 = 1.0f - 2.0f * (xx + zz);
            const float m12 = 2.0f * (yz - wx);

            const float m20 = 2.0f * (xz - wy);
            const float m21 = 2.0f * (yz + wx);
            const float m22 = 1.0f - 2.0f * (xx + yy);

            // Matches FromEuler = qx * qy * qz
            // pitch = rotation around X
            // yaw   = rotation around Y
            // roll  = rotation around Z

            Diligent::float3 euler;

            const float sy = std::clamp(m02, -1.0f, 1.0f);
            euler.y = std::asin(sy);

            const float cosY = std::cos(euler.y);

            // Check for gimbal lock
            if (std::fabs(cosY) > 1e-6f)
            {
                euler.x = std::atan2(-m12, m22);
                euler.z = std::atan2(-m01, m00);
            }
            else
            {
                // Gimbal lock fallback
                // roll set to 0, solve pitch from remaining terms
                euler.x = std::atan2(m21, m11);
                euler.z = 0.0f;
            }

            return euler;
        }

        /// <summary>
        /// Rotates a vector by a quaternion.
        /// </summary>
        /// <param name="q">Rotation quaternion.</param>
        /// <param name="v">Vector to rotate.</param>
        /// <returns>The rotated vector.</returns>
        static Diligent::float3 Rotate(const Quaternion& q, const Diligent::float3& v)
        {
            const Quaternion nq = Normalize(q);
            const Quaternion vq{ v.x, v.y, v.z, 0.0f };

            const Quaternion rq = Multiply(Multiply(nq, vq), Conjugate(nq));
            return Diligent::float3(rq.x, rq.y, rq.z);
        }

        /// <summary>
        /// Returns the forward direction for a rotation.
        /// </summary>
        /// <param name="q">Rotation quaternion.</param>
        /// <returns>The forward direction vector.</returns>
        static Diligent::float3 Foward(const Quaternion& q)
        {
            return Rotate(q, Diligent::float3(0.0f, 0.0f, -1.0f));
        }

        /// <summary>
        /// Returns the right direction for a rotation.
        /// </summary>
        /// <param name="q">Rotation quaternion.</param>
        /// <returns>The right direction vector.</returns>
        static Diligent::float3 Right(const Quaternion& q)
        {
            return Rotate(q, Diligent::float3(1.0f, 0.0f, 0.0f));
        }
    };

    // Transform component storing local and derived world transforms.
    struct TransformComponent
    {
    public:
        /// <summary>
        /// Creates an identity transform.
        /// </summary>
        TransformComponent()
        {
            RebuildLocalMatrixFromLocalTRS();

            m_worldPosition = m_localPosition;
            m_worldRotation = m_localRotation;
            m_worldScale = m_localScale;

            RebuildWorldMatrixFromWorldTRS();
        }

        /// <summary>
        /// Creates a transform from local-space values.
        /// </summary>
        /// <param name="localPosition">Local-space position.</param>
        /// <param name="localRotation">Local-space rotation.</param>
        /// <param name="localScale">Local-space scale.</param>
        /// <returns>A transform initialized from local values.</returns>
        static TransformComponent FromLocal(
            const Diligent::float3& localPosition,
            const Quaternion& localRotation = Quaternion::Identity(),
            const Diligent::float3& localScale = Diligent::float3(1.0f, 1.0f, 1.0f))
        {
            TransformComponent transform;
            transform.m_localPosition = localPosition;
            transform.m_localRotation = Normalize(localRotation);
            transform.m_localScale = localScale;

            transform.RebuildLocalMatrixFromLocalTRS();

            transform.m_worldPosition = transform.m_localPosition;
            transform.m_worldRotation = transform.m_localRotation;
            transform.m_worldScale = transform.m_localScale;

            transform.RebuildWorldMatrixFromWorldTRS();
            return transform;
        }

        /// <summary>
        /// Creates a transform from world-space values.
        /// </summary>
        /// <param name="worldPosition">World-space position.</param>
        /// <param name="worldRotation">World-space rotation.</param>
        /// <param name="worldScale">World-space scale.</param>
        /// <returns>A transform initialized from world values.</returns>
        static TransformComponent FromWorld(
            const Diligent::float3& worldPosition,
            const Quaternion& worldRotation = Quaternion::Identity(),
            const Diligent::float3& worldScale = Diligent::float3(1.0f, 1.0f, 1.0f))
        {
            TransformComponent transform;
            transform.m_localPosition = worldPosition;
            transform.m_localRotation = Normalize(worldRotation);
            transform.m_localScale = worldScale;

            transform.RebuildLocalMatrixFromLocalTRS();

            transform.m_worldPosition = worldPosition;
            transform.m_worldRotation = Normalize(worldRotation);
            transform.m_worldScale = worldScale;

            transform.RebuildWorldMatrixFromWorldTRS();
            return transform;
        }

        /// <summary>
        /// Returns the local position.
        /// </summary>
        /// <returns>The local position.</returns>
        const Diligent::float3& GetLocalPosition() const
        {
            return m_localPosition;
        }

        /// <summary>
        /// Returns the local rotation.
        /// </summary>
        /// <returns>The local rotation.</returns>
        const Quaternion& GetLocalRotation() const
        {
            return m_localRotation;
        }

        /// <summary>
        /// Returns the local scale.
        /// </summary>
        /// <returns>The local scale.</returns>
        const Diligent::float3& GetLocalScale() const
        {
            return m_localScale;
        }

        /// <summary>
        /// Returns the derived world position.
        /// </summary>
        /// <returns>The world position.</returns>
        const Diligent::float3& GetWorldPosition() const
        {
            return m_worldPosition;
        }

        /// <summary>
        /// Returns the derived world rotation.
        /// </summary>
        /// <returns>The world rotation.</returns>
        const Quaternion& GetWorldRotation() const
        {
            return m_worldRotation;
        }

        /// <summary>
        /// Returns the derived world scale.
        /// </summary>
        /// <returns>The world scale.</returns>
        const Diligent::float3& GetWorldScale() const
        {
            return m_worldScale;
        }

        /// <summary>
        /// Returns the local transform matrix.
        /// </summary>
        /// <returns>The local transform matrix.</returns>
        const Diligent::float4x4& GetLocalMatrix() const
        {
            return m_localMatrix;
        }

        /// <summary>
        /// Returns the world transform matrix.
        /// </summary>
        /// <returns>The world transform matrix.</returns>
        const Diligent::float4x4& GetWorldMatrix() const
        {
            return m_worldMatrix;
        }

    private:
        friend flecs::entity GetTransformParent(const flecs::entity& entity);
        friend TransformComponent& RequireTransform(const flecs::entity& entity);

        friend void UpdateWorldFromLocal(const flecs::entity& entity);
        friend void UpdateLocalFromWorld(const flecs::entity& entity);
        friend void UpdateWorldRecursive(const flecs::entity& entity);
        friend void UpdateChildrenRecursive(const flecs::entity& entity);

        friend void SetLocalTransform(
            const flecs::entity& entity,
            const Diligent::float3& localPosition,
            const Quaternion& localRotation,
            const Diligent::float3& localScale);
        friend void SetLocalPosition(const flecs::entity& entity, const Diligent::float3& localPosition);
        friend void SetLocalRotation(const flecs::entity& entity, const Quaternion& localRotation);
        friend void SetLocalScale(const flecs::entity& entity, const Diligent::float3& localScale);

        friend void SetWorldTransform(
            const flecs::entity& entity,
            const Diligent::float3& worldPosition,
            const Quaternion& worldRotation,
            const Diligent::float3& worldScale);
        friend void SetWorldPosition(const flecs::entity& entity, const Diligent::float3& worldPosition);
        friend void SetWorldRotation(const flecs::entity& entity, const Quaternion& worldRotation);
        friend void SetWorldScale(const flecs::entity& entity, const Diligent::float3& worldScale);

        friend void SyncTransformHierarchyFromRoots(flecs::world& world);

        static float Dot(const Quaternion& a, const Quaternion& b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
        }

        static Quaternion Normalize(const Quaternion& q)
        {
            const float lenSq = Dot(q, q);
            if (lenSq <= 0.0f)
            {
                return Quaternion::Identity();
            }

            const float invLen = 1.0f / std::sqrt(lenSq);

            Quaternion result;
            result.x = q.x * invLen;
            result.y = q.y * invLen;
            result.z = q.z * invLen;
            result.w = q.w * invLen;
            return result;
        }

        static Quaternion Conjugate(const Quaternion& q)
        {
            Quaternion result;
            result.x = -q.x;
            result.y = -q.y;
            result.z = -q.z;
            result.w = q.w;
            return result;
        }

        static Quaternion Inverse(const Quaternion& q)
        {
            const float lenSq = Dot(q, q);
            if (lenSq <= 0.0f)
            {
                return Quaternion::Identity();
            }

            const Quaternion c = Conjugate(q);
            const float invLenSq = 1.0f / lenSq;

            Quaternion result;
            result.x = c.x * invLenSq;
            result.y = c.y * invLenSq;
            result.z = c.z * invLenSq;
            result.w = c.w * invLenSq;
            return result;
        }

        static Quaternion Multiply(const Quaternion& a, const Quaternion& b)
        {
            Quaternion result;
            result.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
            result.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
            result.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
            result.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
            return Normalize(result);
        }

        static Diligent::float3 Add(
            const Diligent::float3& a,
            const Diligent::float3& b)
        {
            return Diligent::float3(a.x + b.x, a.y + b.y, a.z + b.z);
        }

        static Diligent::float3 Subtract(
            const Diligent::float3& a,
            const Diligent::float3& b)
        {
            return Diligent::float3(a.x - b.x, a.y - b.y, a.z - b.z);
        }

        static Diligent::float3 MultiplyComponents(
            const Diligent::float3& a,
            const Diligent::float3& b)
        {
            return Diligent::float3(a.x * b.x, a.y * b.y, a.z * b.z);
        }

        static Diligent::float3 DivideSafe(
            const Diligent::float3& value,
            const Diligent::float3& divisor)
        {
            return Diligent::float3(
                divisor.x != 0.0f ? value.x / divisor.x : value.x,
                divisor.y != 0.0f ? value.y / divisor.y : value.y,
                divisor.z != 0.0f ? value.z / divisor.z : value.z);
        }

        static Diligent::float3 RotateVector(const Quaternion& q, const Diligent::float3& v)
        {
            const Quaternion qn = Normalize(q);
            const Quaternion p{ v.x, v.y, v.z, 0.0f };
            const Quaternion rotated = Multiply(Multiply(qn, p), Inverse(qn));
            return Diligent::float3(rotated.x, rotated.y, rotated.z);
        }

        static Diligent::float4x4 MatrixFromQuaternion(const Quaternion& qIn)
        {
            const Quaternion q = Normalize(qIn);

            const float xx = q.x * q.x;
            const float yy = q.y * q.y;
            const float zz = q.z * q.z;
            const float xy = q.x * q.y;
            const float xz = q.x * q.z;
            const float yz = q.y * q.z;
            const float wx = q.w * q.x;
            const float wy = q.w * q.y;
            const float wz = q.w * q.z;

            Diligent::float4x4 m = Diligent::float4x4::Identity();

            m._11 = 1.0f - 2.0f * (yy + zz);
            m._12 = 2.0f * (xy + wz);
            m._13 = 2.0f * (xz - wy);

            m._21 = 2.0f * (xy - wz);
            m._22 = 1.0f - 2.0f * (xx + zz);
            m._23 = 2.0f * (yz + wx);

            m._31 = 2.0f * (xz + wy);
            m._32 = 2.0f * (yz - wx);
            m._33 = 1.0f - 2.0f * (xx + yy);

            return m;
        }

        static Diligent::float4x4 ComposeMatrix(
            const Diligent::float3& position,
            const Quaternion& rotation,
            const Diligent::float3& scale)
        {
            const Diligent::float4x4 S = Diligent::float4x4::Scale(scale.x, scale.y, scale.z);
            const Diligent::float4x4 R = MatrixFromQuaternion(rotation);
            const Diligent::float4x4 T = Diligent::float4x4::Translation(position.x, position.y, position.z);

            return S * R * T;
        }

        void RebuildLocalMatrixFromLocalTRS()
        {
            m_localMatrix = ComposeMatrix(m_localPosition, m_localRotation, m_localScale);
        }

        void RebuildWorldMatrixFromWorldTRS()
        {
            m_worldMatrix = ComposeMatrix(m_worldPosition, m_worldRotation, m_worldScale);
        }

    private:
        Diligent::float3   m_localPosition = Diligent::float3(0.0f, 0.0f, 0.0f);
        Quaternion         m_localRotation = Quaternion::Identity();
        Diligent::float3   m_localScale = Diligent::float3(1.0f, 1.0f, 1.0f);

        Diligent::float3   m_worldPosition = Diligent::float3(0.0f, 0.0f, 0.0f);
        Quaternion         m_worldRotation = Quaternion::Identity();
        Diligent::float3   m_worldScale = Diligent::float3(1.0f, 1.0f, 1.0f);

        Diligent::float4x4 m_localMatrix = Diligent::float4x4::Identity();
        Diligent::float4x4 m_worldMatrix = Diligent::float4x4::Identity();
    };

    /// <summary>
    /// Returns the transform parent for an entity.
    /// </summary>
    /// <param name="entity">Entity to inspect.</param>
    /// <returns>The parent entity handle.</returns>
    inline flecs::entity GetTransformParent(const flecs::entity& entity)
    {
        return entity.parent();
    }

    /// <summary>
    /// Returns a mutable transform and asserts that one exists.
    /// </summary>
    /// <param name="entity">Entity that owns the transform.</param>
    /// <returns>A mutable transform reference.</returns>
    inline TransformComponent& RequireTransform(const flecs::entity& entity)
    {
        TransformComponent* transform = entity.get_mut<TransformComponent>();
        assert(transform != nullptr && "Entity must have TransformComponent");
        return *transform;
    }

    /// <summary>
    /// Recomputes world-space values from local-space values.
    /// </summary>
    /// <param name="entity">Entity whose transform should be updated.</param>
    inline void UpdateWorldFromLocal(const flecs::entity& entity)
    {
        TransformComponent& transform = RequireTransform(entity);
        const flecs::entity parent = GetTransformParent(entity);

        transform.RebuildLocalMatrixFromLocalTRS();

        if (parent.is_valid() && parent.has<TransformComponent>())
        {
            const TransformComponent& parentTransform = *parent.get<TransformComponent>();

            transform.m_worldRotation =
                TransformComponent::Multiply(parentTransform.m_worldRotation, transform.m_localRotation);

            transform.m_worldScale =
                TransformComponent::MultiplyComponents(parentTransform.m_worldScale, transform.m_localScale);

            const Diligent::float3 scaledLocal =
                TransformComponent::MultiplyComponents(parentTransform.m_worldScale, transform.m_localPosition);

            const Diligent::float3 rotatedLocal =
                TransformComponent::RotateVector(parentTransform.m_worldRotation, scaledLocal);

            transform.m_worldPosition =
                TransformComponent::Add(parentTransform.m_worldPosition, rotatedLocal);

            transform.m_worldMatrix = transform.m_localMatrix * parentTransform.m_worldMatrix;
        }
        else
        {
            transform.m_worldPosition = transform.m_localPosition;
            transform.m_worldRotation = transform.m_localRotation;
            transform.m_worldScale = transform.m_localScale;
            transform.m_worldMatrix = transform.m_localMatrix;
        }
    }

    /// <summary>
    /// Recomputes local-space values from world-space values.
    /// </summary>
    /// <param name="entity">Entity whose transform should be updated.</param>
    inline void UpdateLocalFromWorld(const flecs::entity& entity)
    {
        TransformComponent& transform = RequireTransform(entity);
        const flecs::entity parent = GetTransformParent(entity);

        transform.RebuildWorldMatrixFromWorldTRS();

        if (parent.is_valid() && parent.has<TransformComponent>())
        {
            const TransformComponent& parentTransform = *parent.get<TransformComponent>();

            const Quaternion invParentRotation =
                TransformComponent::Inverse(parentTransform.m_worldRotation);

            transform.m_localRotation =
                TransformComponent::Multiply(invParentRotation, transform.m_worldRotation);

            transform.m_localScale =
                TransformComponent::DivideSafe(transform.m_worldScale, parentTransform.m_worldScale);

            const Diligent::float3 worldOffset =
                TransformComponent::Subtract(transform.m_worldPosition, parentTransform.m_worldPosition);

            const Diligent::float3 unrotated =
                TransformComponent::RotateVector(invParentRotation, worldOffset);

            transform.m_localPosition =
                TransformComponent::DivideSafe(unrotated, parentTransform.m_worldScale);
        }
        else
        {
            transform.m_localPosition = transform.m_worldPosition;
            transform.m_localRotation = transform.m_worldRotation;
            transform.m_localScale = transform.m_worldScale;
        }

        transform.RebuildLocalMatrixFromLocalTRS();
    }

    /// <summary>
    /// Updates world transforms for an entity and its descendants.
    /// </summary>
    /// <param name="entity">Root entity to update.</param>
    inline void UpdateWorldRecursive(const flecs::entity& entity)
    {
        UpdateWorldFromLocal(entity);

        entity.children(
            [](flecs::entity child)
            {
                if (child.has<TransformComponent>())
                {
                    UpdateWorldRecursive(child);
                }
            });
    }

    /// <summary>
    /// Updates only the descendants of an entity.
    /// </summary>
    /// <param name="entity">Parent entity whose children should be updated.</param>
    inline void UpdateChildrenRecursive(const flecs::entity& entity)
    {
        entity.children(
            [](flecs::entity child)
            {
                if (child.has<TransformComponent>())
                {
                    UpdateWorldRecursive(child);
                }
            });
    }

    /// <summary>
    /// Sets the full local transform and updates the hierarchy.
    /// </summary>
    /// <param name="entity">Entity to update.</param>
    /// <param name="localPosition">New local position.</param>
    /// <param name="localRotation">New local rotation.</param>
    /// <param name="localScale">New local scale.</param>
    inline void SetLocalTransform(
        const flecs::entity& entity,
        const Diligent::float3& localPosition,
        const Quaternion& localRotation,
        const Diligent::float3& localScale)
    {
        TransformComponent& transform = RequireTransform(entity);

        transform.m_localPosition = localPosition;
        transform.m_localRotation = TransformComponent::Normalize(localRotation);
        transform.m_localScale = localScale;

        UpdateWorldRecursive(entity);
    }

    /// <summary>
    /// Sets local position and updates the hierarchy.
    /// </summary>
    /// <param name="entity">Entity to update.</param>
    /// <param name="localPosition">New local position.</param>
    inline void SetLocalPosition(const flecs::entity& entity, const Diligent::float3& localPosition)
    {
        TransformComponent& transform = RequireTransform(entity);
        transform.m_localPosition = localPosition;
        UpdateWorldRecursive(entity);
    }

    /// <summary>
    /// Sets local rotation and updates the hierarchy.
    /// </summary>
    /// <param name="entity">Entity to update.</param>
    /// <param name="localRotation">New local rotation.</param>
    inline void SetLocalRotation(const flecs::entity& entity, const Quaternion& localRotation)
    {
        TransformComponent& transform = RequireTransform(entity);
        transform.m_localRotation = TransformComponent::Normalize(localRotation);
        UpdateWorldRecursive(entity);
    }

    /// <summary>
    /// Sets local scale and updates the hierarchy.
    /// </summary>
    /// <param name="entity">Entity to update.</param>
    /// <param name="localScale">New local scale.</param>
    inline void SetLocalScale(const flecs::entity& entity, const Diligent::float3& localScale)
    {
        TransformComponent& transform = RequireTransform(entity);
        transform.m_localScale = localScale;
        UpdateWorldRecursive(entity);
    }

    /// <summary>
    /// Sets the full world transform and updates the hierarchy.
    /// </summary>
    /// <param name="entity">Entity to update.</param>
    /// <param name="worldPosition">New world position.</param>
    /// <param name="worldRotation">New world rotation.</param>
    /// <param name="worldScale">New world scale.</param>
    inline void SetWorldTransform(
        const flecs::entity& entity,
        const Diligent::float3& worldPosition,
        const Quaternion& worldRotation,
        const Diligent::float3& worldScale)
    {
        TransformComponent& transform = RequireTransform(entity);

        transform.m_worldPosition = worldPosition;
        transform.m_worldRotation = TransformComponent::Normalize(worldRotation);
        transform.m_worldScale = worldScale;

        UpdateLocalFromWorld(entity);
        UpdateChildrenRecursive(entity);
    }

    /// <summary>
    /// Sets world position and updates the hierarchy.
    /// </summary>
    /// <param name="entity">Entity to update.</param>
    /// <param name="worldPosition">New world position.</param>
    inline void SetWorldPosition(const flecs::entity& entity, const Diligent::float3& worldPosition)
    {
        TransformComponent& transform = RequireTransform(entity);
        transform.m_worldPosition = worldPosition;
        UpdateLocalFromWorld(entity);
        UpdateChildrenRecursive(entity);
    }

    /// <summary>
    /// Sets world rotation and updates the hierarchy.
    /// </summary>
    /// <param name="entity">Entity to update.</param>
    /// <param name="worldRotation">New world rotation.</param>
    inline void SetWorldRotation(const flecs::entity& entity, const Quaternion& worldRotation)
    {
        TransformComponent& transform = RequireTransform(entity);
        transform.m_worldRotation = TransformComponent::Normalize(worldRotation);
        UpdateLocalFromWorld(entity);
        UpdateChildrenRecursive(entity);
    }

    /// <summary>
    /// Sets world scale and updates the hierarchy.
    /// </summary>
    /// <param name="entity">Entity to update.</param>
    /// <param name="worldScale">New world scale.</param>
    inline void SetWorldScale(const flecs::entity& entity, const Diligent::float3& worldScale)
    {
        TransformComponent& transform = RequireTransform(entity);
        transform.m_worldScale = worldScale;
        UpdateLocalFromWorld(entity);
        UpdateChildrenRecursive(entity);
    }

    /// <summary>
    /// Rebuilds transform state for all root entities in a world.
    /// </summary>
    /// <param name="world">World whose transform hierarchy should be synchronized.</param>
    inline void SyncTransformHierarchyFromRoots(flecs::world& world)
    {
        world.each<TransformComponent>(
            [](flecs::entity entity, TransformComponent&)
            {
                const flecs::entity parent = GetTransformParent(entity);
                const bool isRoot = !parent.is_valid() || !parent.has<TransformComponent>();

                if (isRoot)
                {
                    UpdateWorldRecursive(entity);
                }
            });
    }

} // namespace NexusEngine