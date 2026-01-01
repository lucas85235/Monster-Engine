#include "editor/ColliderDebugRenderer.h"

#include <algorithm>
#include <filesystem>
#include <vector>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include "core/PrimitiveFactory.h"
#include "engine/Log.h"
#include "engine/resources/MaterialManager.h"
#include "engine/ecs/SimpleComponents.h"

namespace mst {

namespace fs = std::filesystem;

static constexpr int SPHERE_SEGMENTS = 24;
static constexpr int CAPSULE_SEGMENTS = 16;
static constexpr float PI = 3.14159265358979323846f;

ColliderDebugRenderer::ColliderDebugRenderer() {
    CreateShader();
    CreateBoxMesh();
    CreateSphereMesh();
    CreateCapsuleMesh();
}

ColliderDebugRenderer::~ColliderDebugRenderer() {
    if (boxVao_) glDeleteVertexArrays(1, &boxVao_);
    if (boxVbo_) glDeleteBuffers(1, &boxVbo_);
    if (boxEbo_) glDeleteBuffers(1, &boxEbo_);
    if (sphereVao_) glDeleteVertexArrays(1, &sphereVao_);
    if (sphereVbo_) glDeleteBuffers(1, &sphereVbo_);
    if (sphereEbo_) glDeleteBuffers(1, &sphereEbo_);
    if (capsuleVao_) glDeleteVertexArrays(1, &capsuleVao_);
    if (capsuleVbo_) glDeleteBuffers(1, &capsuleVbo_);
    if (capsuleEbo_) glDeleteBuffers(1, &capsuleEbo_);
}

void ColliderDebugRenderer::CreateShader() {
    fs::path assetsPath = fs::current_path() / "assets";
    fs::path vertPath = assetsPath / "shaders" / "utility" / "collider_debug.vert";
    fs::path fragPath = assetsPath / "shaders" / "utility" / "collider_debug.frag";

    if (!fs::exists(vertPath) || !fs::exists(fragPath)) {
        SE_LOG_ERROR("ColliderDebugRenderer: Shaders not found at {}", vertPath.string());
        return;
    }

    shader_ = se::MaterialManager::GetShader("ColliderDebugShader", vertPath, fragPath);
    if (shader_) {
        SE_LOG_INFO("ColliderDebugRenderer: Loaded collider debug shader");
    } else {
        SE_LOG_ERROR("ColliderDebugRenderer: Failed to load collider debug shader");
    }
}

void ColliderDebugRenderer::CreateBoxMesh() {
    // Unit cube wireframe (will be scaled at render time)
    float vertices[] = {
        // Bottom face
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        // Top face
        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f
    };

    unsigned int indices[] = {
        // Bottom
        0, 1, 1, 2, 2, 3, 3, 0,
        // Top
        4, 5, 5, 6, 6, 7, 7, 4,
        // Verticals
        0, 4, 1, 5, 2, 6, 3, 7
    };

    boxIndexCount_ = 24;

    glGenVertexArrays(1, &boxVao_);
    glGenBuffers(1, &boxVbo_);
    glGenBuffers(1, &boxEbo_);

    glBindVertexArray(boxVao_);
    glBindBuffer(GL_ARRAY_BUFFER, boxVbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, boxEbo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    SE_LOG_INFO("ColliderDebugRenderer: Created box wireframe mesh");
}

void ColliderDebugRenderer::CreateSphereMesh() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    unsigned int currentIndex = 0;

    // Create 3 circles (XY, XZ, YZ planes)
    for (int plane = 0; plane < 3; ++plane) {
        unsigned int startIndex = currentIndex;
        for (int i = 0; i < SPHERE_SEGMENTS; ++i) {
            float angle = 2.0f * PI * i / SPHERE_SEGMENTS;
            float x = cosf(angle) * 0.5f;
            float y = sinf(angle) * 0.5f;
            
            if (plane == 0) {  // XY plane
                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(0.0f);
            } else if (plane == 1) {  // XZ plane
                vertices.push_back(x);
                vertices.push_back(0.0f);
                vertices.push_back(y);
            } else {  // YZ plane
                vertices.push_back(0.0f);
                vertices.push_back(x);
                vertices.push_back(y);
            }
            
            indices.push_back(currentIndex);
            indices.push_back(startIndex + ((i + 1) % SPHERE_SEGMENTS));
            currentIndex++;
        }
    }

    sphereIndexCount_ = static_cast<int>(indices.size());

    glGenVertexArrays(1, &sphereVao_);
    glGenBuffers(1, &sphereVbo_);
    glGenBuffers(1, &sphereEbo_);

    glBindVertexArray(sphereVao_);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEbo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    SE_LOG_INFO("ColliderDebugRenderer: Created sphere wireframe mesh");
}

void ColliderDebugRenderer::CreateCapsuleMesh() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    unsigned int currentIndex = 0;

    // Top hemisphere circle (XZ)
    for (int i = 0; i < CAPSULE_SEGMENTS; ++i) {
        float angle = 2.0f * PI * i / CAPSULE_SEGMENTS;
        vertices.push_back(cosf(angle) * 0.5f);
        vertices.push_back(0.5f);
        vertices.push_back(sinf(angle) * 0.5f);
        indices.push_back(currentIndex);
        indices.push_back(currentIndex + ((i + 1) % CAPSULE_SEGMENTS) - i + i);
        currentIndex++;
    }
    // Fix indices for top circle
    for (int i = 0; i < CAPSULE_SEGMENTS; ++i) {
        indices[i * 2] = i;
        indices[i * 2 + 1] = (i + 1) % CAPSULE_SEGMENTS;
    }

    // Bottom hemisphere circle (XZ)
    unsigned int bottomStart = currentIndex;
    for (int i = 0; i < CAPSULE_SEGMENTS; ++i) {
        float angle = 2.0f * PI * i / CAPSULE_SEGMENTS;
        vertices.push_back(cosf(angle) * 0.5f);
        vertices.push_back(-0.5f);
        vertices.push_back(sinf(angle) * 0.5f);
        indices.push_back(currentIndex);
        indices.push_back(bottomStart + ((i + 1) % CAPSULE_SEGMENTS));
        currentIndex++;
    }

    // 4 vertical lines connecting top and bottom
    for (int i = 0; i < 4; ++i) {
        int idx = i * (CAPSULE_SEGMENTS / 4);
        indices.push_back(idx);                   // Top vertex
        indices.push_back(bottomStart + idx);     // Bottom vertex
    }

    // Top hemisphere arc (XY plane, upper half)
    unsigned int topArcStart = currentIndex;
    for (int i = 0; i <= CAPSULE_SEGMENTS / 2; ++i) {
        float angle = PI * i / (CAPSULE_SEGMENTS / 2);
        vertices.push_back(sinf(angle) * 0.5f);
        vertices.push_back(0.5f + cosf(angle) * 0.5f);
        vertices.push_back(0.0f);
        if (i > 0) {
            indices.push_back(currentIndex - 1);
            indices.push_back(currentIndex);
        }
        currentIndex++;
    }

    // Bottom hemisphere arc (XY plane, lower half)
    for (int i = 0; i <= CAPSULE_SEGMENTS / 2; ++i) {
        float angle = PI + PI * i / (CAPSULE_SEGMENTS / 2);
        vertices.push_back(sinf(angle) * 0.5f);
        vertices.push_back(-0.5f + cosf(angle) * 0.5f);
        vertices.push_back(0.0f);
        if (i > 0) {
            indices.push_back(currentIndex - 1);
            indices.push_back(currentIndex);
        }
        currentIndex++;
    }

    capsuleIndexCount_ = static_cast<int>(indices.size());

    glGenVertexArrays(1, &capsuleVao_);
    glGenBuffers(1, &capsuleVbo_);
    glGenBuffers(1, &capsuleEbo_);

    glBindVertexArray(capsuleVao_);
    glBindBuffer(GL_ARRAY_BUFFER, capsuleVbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, capsuleEbo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    SE_LOG_INFO("ColliderDebugRenderer: Created capsule wireframe mesh");
}

void ColliderDebugRenderer::Render(const glm::mat4& view, const glm::mat4& projection, se::Scene& scene) {
    if (!shader_) return;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glLineWidth(2.0f);

    shader_->bind();

    glm::mat4 viewProj = projection * view;

    auto entities = scene.GetAllEntitiesWith<se::TransformComponent, PrimitiveFactory::EditorMetadata>();
    for (auto entityHandle : entities) {
        se::Entity entity(entityHandle, &scene);
        auto& metadata = entity.GetComponent<PrimitiveFactory::EditorMetadata>();
        
        if (!metadata.hasCollision || metadata.colliderType == ColliderType::None) continue;

        auto& transform = entity.GetComponent<se::TransformComponent>();

        switch (metadata.colliderType) {
            case ColliderType::Box:
                RenderBox(viewProj, transform.Position, transform.Rotation, 
                          metadata.colliderSize * transform.Scale);
                break;
            case ColliderType::Sphere: {
                // Use the largest scale component for sphere radius
                float maxScale = std::max({transform.Scale.x, transform.Scale.y, transform.Scale.z});
                RenderSphere(viewProj, transform.Position, metadata.colliderRadius * maxScale);
                break;
            }
            case ColliderType::Capsule:
                RenderCapsule(viewProj, transform.Position, transform.Rotation, 
                              metadata.colliderRadius * std::max(transform.Scale.x, transform.Scale.z),
                              metadata.colliderHeight * transform.Scale.y);
                break;
            default:
                break;
        }
    }

    glLineWidth(1.0f);
}

void ColliderDebugRenderer::RenderBox(const glm::mat4& viewProj, const glm::vec3& position, 
                                       const glm::vec3& rotation, const glm::vec3& size) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1, 0, 0));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0, 1, 0));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0, 0, 1));
    model = glm::scale(model, size);

    glm::mat4 mvp = viewProj * model;
    shader_->setMat4("uMVP", mvp);
    shader_->setVec3("uColor", boxColor_);

    glBindVertexArray(boxVao_);
    glDrawElements(GL_LINES, boxIndexCount_, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void ColliderDebugRenderer::RenderSphere(const glm::mat4& viewProj, const glm::vec3& position, float radius) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, glm::vec3(radius * 2.0f));

    glm::mat4 mvp = viewProj * model;
    shader_->setMat4("uMVP", mvp);
    shader_->setVec3("uColor", sphereColor_);

    glBindVertexArray(sphereVao_);
    glDrawElements(GL_LINES, sphereIndexCount_, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void ColliderDebugRenderer::RenderCapsule(const glm::mat4& viewProj, const glm::vec3& position, 
                                           const glm::vec3& rotation, float radius, float height) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1, 0, 0));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0, 1, 0));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0, 0, 1));
    // Scale: radius for X/Z, height for Y
    model = glm::scale(model, glm::vec3(radius * 2.0f, height, radius * 2.0f));

    glm::mat4 mvp = viewProj * model;
    shader_->setMat4("uMVP", mvp);
    shader_->setVec3("uColor", capsuleColor_);

    glBindVertexArray(capsuleVao_);
    glDrawElements(GL_LINES, capsuleIndexCount_, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

}  // namespace mst
