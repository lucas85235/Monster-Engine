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
// Similar to Unity's Transform component
struct TransformComponent {
    glm::vec3 Position = {0.0f, 0.0f, 0.0f};
    glm::vec3 Rotation = {0.0f, 0.0f, 0.0f};  // Euler angles in degrees
    glm::vec3 Scale    = {1.0f, 1.0f, 1.0f};

    TransformComponent() = default;

    TransformComponent(const TransformComponent&) = default;

    TransformComponent(const glm::vec3& position) : Position(position) {}

    // Get the transformation matrix
    glm::mat4 GetTransform() const {
        glm::mat4 rotation = glm::toMat4(glm::quat(glm::radians(Rotation)));
        return glm::translate(glm::mat4(1.0f), Position) * rotation * glm::scale(glm::mat4(1.0f), Scale);
    }

    // Set position
    void SetPosition(const glm::vec3& position) {
        Position = position;
    }

    // Set rotation (in degrees)
    void SetRotation(const glm::vec3& rotation) {
        Rotation = rotation;
    }

    // Set scale
    void SetScale(const glm::vec3& scale) {
        Scale = scale;
    }

    // Translate by offset
    void Translate(const glm::vec3& offset) {
        Position += offset;
    }

    // Rotate by offset (in degrees)
    void Rotate(const glm::vec3& offset) {
        Rotation += offset;
    }

    // Get forward vector
    glm::vec3 GetForward() const {
        glm::quat quat = glm::quat(glm::radians(Rotation));
        return glm::rotate(quat, glm::vec3(0.0f, 0.0f, -1.0f));
    }

    // Get right vector
    glm::vec3 GetRight() const {
        glm::quat quat = glm::quat(glm::radians(Rotation));
        return glm::rotate(quat, glm::vec3(1.0f, 0.0f, 0.0f));
    }

    // Get up vector
    glm::vec3 GetUp() const {
        glm::quat quat = glm::quat(glm::radians(Rotation));
        return glm::rotate(quat, glm::vec3(0.0f, 1.0f, 0.0f));
    }
};

// ==================== Name Component ====================
// Gives each entity a human-readable name
struct NameComponent {
    std::string Name;

    NameComponent() = default;

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
// Handles mesh rendering for an entity
struct MeshRenderComponent {
    std::shared_ptr<VertexArray> VertexArray;
    std::shared_ptr<Material>    Material;
    bool                         IsVisible      = true;
    bool                         CastShadows    = true;
    bool                         ReceiveShadows = true;

    MeshRenderComponent() = default;

    MeshRenderComponent(const MeshRenderComponent&) = default;

    MeshRenderComponent(std::shared_ptr<se::VertexArray> va, std::shared_ptr<se::Material> mat) : VertexArray(va), Material(mat) {}
};

struct DirectionalLightComponent {
    glm::vec3 Color{1.0f, 1.0f, 1.0f};
    float     Intensity   = 1.0f;
    bool      Enabled     = true;
    bool      CastShadows = true;

    DirectionalLightComponent() = default;

    DirectionalLightComponent(const DirectionalLightComponent&) = default;
};
}  // namespace se
