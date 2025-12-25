#pragma once
/**
 * ColliderDebugRenderer.h - Wireframe debug visualization for colliders.
 *
 * Renders box, sphere, and capsule collider shapes as colored wireframes
 * using OpenGL lines for editor visualization.
 */

#include <memory>
#include <glad/glad.h>
#include <glm.hpp>

#include "core/MapData.h"
#include "engine/Shader.h"
#include "engine/ecs/Scene.h"

namespace mst {

class ColliderDebugRenderer {
   public:
    ColliderDebugRenderer();
    ~ColliderDebugRenderer();

    void Render(const glm::mat4& view, const glm::mat4& projection, se::Scene& scene);

    void SetBoxColor(const glm::vec3& color) { boxColor_ = color; }
    void SetSphereColor(const glm::vec3& color) { sphereColor_ = color; }
    void SetCapsuleColor(const glm::vec3& color) { capsuleColor_ = color; }

   private:
    void CreateShader();
    void CreateBoxMesh();
    void CreateSphereMesh();
    void CreateCapsuleMesh();

    void RenderBox(const glm::mat4& viewProj, const glm::vec3& position, 
                   const glm::vec3& rotation, const glm::vec3& size);
    void RenderSphere(const glm::mat4& viewProj, const glm::vec3& position, float radius);
    void RenderCapsule(const glm::mat4& viewProj, const glm::vec3& position, 
                       const glm::vec3& rotation, float radius, float height);

    std::shared_ptr<se::Shader> shader_;
    
    // Box wireframe
    GLuint boxVao_ = 0;
    GLuint boxVbo_ = 0;
    GLuint boxEbo_ = 0;
    int boxIndexCount_ = 0;

    // Sphere wireframe
    GLuint sphereVao_ = 0;
    GLuint sphereVbo_ = 0;
    GLuint sphereEbo_ = 0;
    int sphereIndexCount_ = 0;

    // Capsule wireframe
    GLuint capsuleVao_ = 0;
    GLuint capsuleVbo_ = 0;
    GLuint capsuleEbo_ = 0;
    int capsuleIndexCount_ = 0;

    glm::vec3 boxColor_{0.0f, 1.0f, 0.0f};
    glm::vec3 sphereColor_{0.0f, 1.0f, 1.0f};
    glm::vec3 capsuleColor_{1.0f, 1.0f, 0.0f};
};

}  // namespace mst
