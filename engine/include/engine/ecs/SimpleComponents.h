#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtx/quaternion.hpp>
#include <memory>
#include <string>
#include <vector>

namespace se {
// Forward declarations
class Material;
class VertexArray;

// ==================== Transform Component ====================
// Similar to Unity's Transform component with cached matrix optimization
struct TransformComponent {
    TransformComponent() = default;

    TransformComponent(const TransformComponent& other)
        : Position(other.Position),
          Rotation(other.Rotation),
          Scale(other.Scale),
          cachedTransform_(other.cachedTransform_),
          dirty_(other.dirty_) {}

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
        cachedTransform_ =
            glm::translate(Matrix4(1.0f), Position) * rotation * glm::scale(Matrix4(1.0f), Scale);
        dirty_ = false;
    }

    mutable Matrix4    cachedTransform_{1.0f};
    mutable Quaternion cachedQuaternion_;
    mutable bool       dirty_                 = true;
    mutable bool       cachedQuaternionValid_ = false;

   public:
    // World transformation matrix (calculated by hierarchy system)
    Matrix4 WorldMatrix{1.0f};
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
struct TextureMaterial;  // Forward declaration
class MaterialInstance;  // Forward declaration (new material system)

struct MeshRenderComponent {
    std::shared_ptr<VertexArray> vertex_array;
    std::shared_ptr<Material>    material;
    Vector4                      Color{1.0f, 1.0f, 1.0f, 1.0f};  // Per-instance color
    bool                         IsVisible      = true;
    bool                         CastShadows    = true;
    bool                         ReceiveShadows = true;
    
    // Emissive properties for GI
    Vector3                      EmissiveColor{0.0f, 0.0f, 0.0f};  // RGB emissive color
    float                        EmissiveFactor = 0.0f;             // Emission intensity multiplier
    
    // New unified material system
    MaterialInstance*            materialInstance = nullptr;  // Non-owning, from MaterialLibrary
    
    // Legacy PBR Material parameters (kept for backward compat)
    float                        Metallic = 0.0f;       // 0 = dielectric, 1 = metal
    float                        Roughness = 0.5f;      // Perceptual roughness [0-1]
    float                        Reflectance = 0.5f;    // Dielectric reflectance (0.5 = 4% F0)
    float                        AO = 1.0f;             // Ambient occlusion multiplier
    bool                         UseCustomPBR = false;  // Use custom PBR instead of defaults
    
    // Legacy PBR Texture material (kept for backward compat)
    std::shared_ptr<TextureMaterial> customTextureMaterial;

    MeshRenderComponent()                           = default;
    MeshRenderComponent(const MeshRenderComponent&) = default;
    MeshRenderComponent(std::shared_ptr<se::VertexArray> va, std::shared_ptr<se::Material> mat)
        : vertex_array(va), material(mat) {}
    MeshRenderComponent(std::shared_ptr<se::VertexArray> va, std::shared_ptr<se::Material> mat,
                        const Vector4& color)
        : vertex_array(va), material(mat), Color(color) {}
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

// ==================== Camera Component ====================
enum class CameraMode {
    FreeFly,       // Free-fly editor-style camera (WASD + mouse look)
    ThirdPerson,   // Third-person camera following a target entity (uses SpringArmComponent)
    Orbit,         // Orbit camera around a target point
    Fixed          // Fixed/cinematic camera (no input processing)
};

struct CameraComponent {
    CameraMode Mode = CameraMode::FreeFly;

    // Projection settings
    float FOV       = 60.0f;    // Field of view in degrees
    float NearPlane = 0.1f;
    float FarPlane  = 500.0f;

    // Is this the main (active) camera for the scene?
    bool IsMain = false;

    // Input sensitivity
    float LookSensitivityX = 0.1f;
    float LookSensitivityY = 0.1f;
    bool  InvertY          = false;

    // Movement speed (FreeFly mode)
    float MoveSpeed   = 5.0f;
    float SprintSpeed = 15.0f;

    // Target entity for ThirdPerson/Orbit modes
    entt::entity TargetEntity = entt::null;

    // Internal state (managed by CameraSystem, not set by user)
    float Yaw   = 0.0f;
    float Pitch = 0.0f;

    CameraComponent()                       = default;
    CameraComponent(const CameraComponent&) = default;
    explicit CameraComponent(CameraMode mode) : Mode(mode) {}
};

// ==================== Point Light Component ====================
struct PointLightComponent {
    Vector3 Color{1.0f, 1.0f, 1.0f};
    float   Intensity = 100000.0f;
    float   Falloff   = 10.0f;
    bool    Enabled   = true;

    // Internal handle for LightSyncSystem (user should not modify)
    size_t InternalIndex = SIZE_MAX;

    PointLightComponent()                           = default;
    PointLightComponent(const PointLightComponent&) = default;
};

// ==================== Relationship Component ====================
struct RelationshipComponent {
    entt::entity              Parent = entt::null;
    std::vector<entt::entity> Children;

    size_t ChildrenCount() const {
        return Children.size();
    }
};

}  // namespace se

