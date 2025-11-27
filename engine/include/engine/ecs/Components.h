#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtx/quaternion.hpp>
#include <memory>
#include <string>

namespace se
{
    // Forward declarations
    class Material;
    class VertexArray;

    // ==================== Transform Component ====================
    // Similar to Unity's Transform component
    struct TransformComponent
    {
        Vector3 Position = {0.0f, 0.0f, 0.0f};
        Vector3 Rotation = {0.0f, 0.0f, 0.0f}; // Euler angles in degrees
        Vector3 Scale = {1.0f, 1.0f, 1.0f};

        TransformComponent() = default;

        TransformComponent(const TransformComponent&) = default;

        TransformComponent(const Vector3& position) : Position(position)
        {
        }

        // Get the transformation matrix
        Matrix4 GetTransform() const
        {
            Matrix4 rotation = glm::toMat4(Quaternion(glm::radians(Rotation)));
            return glm::translate(Matrix4(1.0f), Position) * rotation * glm::scale(Matrix4(1.0f), Scale);
        }

        // Set position
        void SetPosition(const Vector3& position)
        {
            Position = position;
        }

        // Set rotation (in degrees)
        void SetRotation(const Vector3& rotation)
        {
            Rotation = rotation;
        }

        // Set scale
        void SetScale(const Vector3& scale)
        {
            Scale = scale;
        }

        // Translate by offset
        void Translate(const Vector3& offset)
        {
            Position += offset;
        }

        // Rotate by offset (in degrees)
        void Rotate(const Vector3& offset)
        {
            Rotation += offset;
        }

        // Get forward vector
        Vector3 GetForward() const
        {
            Quaternion quat = Quaternion(glm::radians(Rotation));
            return glm::rotate(quat, Vector3(0.0f, 0.0f, -1.0f));
        }

        // Get right vector
        Vector3 GetRight() const
        {
            Quaternion quat = Quaternion(glm::radians(Rotation));
            return glm::rotate(quat, Vector3(1.0f, 0.0f, 0.0f));
        }

        // Get up vector
        Vector3 GetUp() const
        {
            Quaternion quat = Quaternion(glm::radians(Rotation));
            return glm::rotate(quat, Vector3(0.0f, 1.0f, 0.0f));
        }
    };

    // ==================== Name Component ====================
    // Gives each entity a human-readable name
    struct NameComponent
    {
        std::string Name;

        NameComponent() = default;

        NameComponent(const NameComponent&) = default;

        NameComponent(const std::string& name) : Name(name)
        {
        }

        operator const std::string&() const
        {
            return Name;
        }

        operator std::string&()
        {
            return Name;
        }
    };

    // ==================== Mesh Render Component ====================
    // Handles mesh rendering for an entity
    struct MeshRenderComponent
    {
        std::shared_ptr<VertexArray> VertexArray;
        std::shared_ptr<Material> Material;
        bool IsVisible = true;
        bool CastShadows = true;
        bool ReceiveShadows = true;

        MeshRenderComponent() = default;

        MeshRenderComponent(const MeshRenderComponent&) = default;

        MeshRenderComponent(std::shared_ptr<se::VertexArray> va, std::shared_ptr<se::Material> mat) : VertexArray(va), Material(mat)
        {
        }
    };

    struct DirectionalLightComponent
    {
        Vector3 Color{1.0f, 1.0f, 1.0f};
        float Intensity = 1.0f;
        bool Enabled = true;
        bool CastShadows = true;

        DirectionalLightComponent() = default;

        DirectionalLightComponent(const DirectionalLightComponent&) = default;
    };

    // ==================== Spring Arm Component ====================
    // Controls camera distance and rotation relative to target
    struct SpringArmComponent
    {
        float TargetArmLength = 5.0f;
        Vector3 SocketOffset = {0.0f, 0.0f, 0.0f};
        bool UsePawnControlRotation = true;

        // Camera rotation (controlled by input)
        float Pitch = -20.0f; // Start looking slightly down
        float Yaw = 0.0f;

        // Constraints
        float MinPitch = -80.0f;
        float MaxPitch = 80.0f;

        SpringArmComponent() = default;
        SpringArmComponent(const SpringArmComponent&) = default;
    };

    struct RigidbodyData
    {
        RigidbodyData() = default;
        float mass = 1.0f;
        float gravityScale = 1.0f;
    };

    struct RigidBodyComponent
    {
        RigidBodyComponent(const RigidbodyData data)
        {
            data_ = data;
            PrintName();
        }

        void PrintName()
        {
            SE_LOG_ERROR("RigidBody component initialized");
        }

    private:
        RigidbodyData data_;
    };
} // namespace se
