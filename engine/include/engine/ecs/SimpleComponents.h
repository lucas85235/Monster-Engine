#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtx/quaternion.hpp>
#include <memory>
#include <string>

namespace se {
// Forward declarations
class Material;
class VertexArray;

// ==================== Transform Component ====================
// Similar to Unity's Transform component with cached matrix optimization
struct TransformComponent {
    TransformComponent() = default;

    TransformComponent(const TransformComponent& other)
        : Position(other.Position), Rotation(other.Rotation), Scale(other.Scale), cachedTransform_(other.cachedTransform_), dirty_(other.dirty_) {}

    TransformComponent(const Vector3& position) : Position(position), dirty_(true) {}

    // Get the transformation matrix (uses cache when possible)
    Matrix4 GetTransform() const {
        if (dirty_) { UpdateCache(); }
        return cachedTransform_;
    }

    // Set position (marks dirty)
    void SetPosition(const Vector3& position) {
        if (Position != position) {
            Position = position;
            dirty_   = true;
        }
    }

    // Set rotation in degrees (marks dirty)
    void SetRotation(const Vector3& rotation) {
        if (Rotation != rotation) {
            Rotation               = rotation;
            dirty_                 = true;
            cachedQuaternionValid_ = false;
        }
    }

    // Set scale (marks dirty)
    void SetScale(const Vector3& scale) {
        if (Scale != scale) {
            Scale  = scale;
            dirty_ = true;
        }
    }

    // Translate by offset (marks dirty)
    void Translate(const Vector3& offset) {
        Position += offset;
        dirty_ = true;
    }

    // Rotate by offset in degrees (marks dirty)
    void Rotate(const Vector3& offset) {
        Rotation += offset;
        dirty_                 = true;
        cachedQuaternionValid_ = false;
    }

    // Get forward vector (uses cached quaternion when possible)
    Vector3 GetForward() const {
        return glm::rotate(GetQuaternion(), Vector3(0.0f, 0.0f, -1.0f));
    }

    // Get right vector
    Vector3 GetRight() const {
        return glm::rotate(GetQuaternion(), Vector3(1.0f, 0.0f, 0.0f));
    }

    // Get up vector
    Vector3 GetUp() const {
        return glm::rotate(GetQuaternion(), Vector3(0.0f, 1.0f, 0.0f));
    }

    // Get quaternion representation
    Quaternion GetQuaternion() const {
        if (!cachedQuaternionValid_) {
            cachedQuaternion_      = Quaternion(glm::radians(Rotation));
            cachedQuaternionValid_ = true;
        }
        return cachedQuaternion_;
    }

    // Force cache invalidation (for external modifications to Position/Rotation/Scale)
    void MarkDirty() {
        dirty_                 = true;
        cachedQuaternionValid_ = false;
    }

    // Public transform data
    Vector3 Position = {0.0f, 0.0f, 0.0f};
    Vector3 Rotation = {0.0f, 0.0f, 0.0f};  // Euler angles in degrees
    Vector3 Scale    = {1.0f, 1.0f, 1.0f};

   private:
    void UpdateCache() const {
        Matrix4 rotation = glm::toMat4(GetQuaternion());
        cachedTransform_ = glm::translate(Matrix4(1.0f), Position) * rotation * glm::scale(Matrix4(1.0f), Scale);
        dirty_           = false;
    }

    mutable Matrix4    cachedTransform_{1.0f};
    mutable Quaternion cachedQuaternion_;
    mutable bool       dirty_                 = true;
    mutable bool       cachedQuaternionValid_ = false;
};

// ==================== Name Component ====================
struct NameComponent {
    std::string Name;

    NameComponent()                     = default;
    NameComponent(const NameComponent&) = default;
    NameComponent(const std::string& name) : Name(name) {}

    operator const std::string&() const {
        return Name;
    }
    operator std::string&() {
        return Name;
    }
};

// ==================== Mesh Render Component ====================
struct MeshRenderComponent {
    std::shared_ptr<VertexArray> MeshVertexArray;
    std::shared_ptr<Material>    MeshMaterial;
    Vector4                      Color{1.0f, 1.0f, 1.0f, 1.0f};  // Per-instance color
    bool                         IsVisible      = true;
    bool                         CastShadows    = true;
    bool                         ReceiveShadows = true;

    MeshRenderComponent()                           = default;
    MeshRenderComponent(const MeshRenderComponent&) = default;
    MeshRenderComponent(std::shared_ptr<se::VertexArray> va, std::shared_ptr<se::Material> mat) : MeshVertexArray(va), MeshMaterial(mat) {}
    MeshRenderComponent(std::shared_ptr<se::VertexArray> va, std::shared_ptr<se::Material> mat, const Vector4& color)
        : MeshVertexArray(va), MeshMaterial(mat), Color(color) {}

    float BoundingRadius = 1.0f;
};

// ==================== Directional Light Component ====================
struct DirectionalLightComponent {
    Vector3 Color{1.0f, 1.0f, 1.0f};
    float   Intensity   = 1.0f;
    bool    Enabled     = true;
    bool    CastShadows = true;

    DirectionalLightComponent()                                 = default;
    DirectionalLightComponent(const DirectionalLightComponent&) = default;
};

// ==================== Spring Arm Component ====================
struct SpringArmComponent {
    float   TargetArmLength = 5.0f;
    Vector3 SocketOffset    = {0.0f, 0.0f, 0.0f};

    float Pitch = -20.0f;
    float Yaw   = 0.0f;

    float MinPitch = -80.0f;
    float MaxPitch = 80.0f;

    bool  DoCollisionTest = true;
    float ProbeSize       = 0.12f;
    float CollisionLag    = 0.0f;

    mutable float CurrentArmLength = 5.0f;

    SpringArmComponent()                          = default;
    SpringArmComponent(const SpringArmComponent&) = default;
};

}  // namespace se
