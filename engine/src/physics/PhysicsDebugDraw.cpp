#include "engine/physics/PhysicsDebugDraw.h"

#include <glad/glad.h>

#include <iostream>

#include "engine/Application.h"
#include "engine/Log.h"

namespace se {

PhysicsDebugDraw::PhysicsDebugDraw() {
    debug_mode_ = DBG_DrawWireframe;
    //DBG_NoDebug

    const std::string vertexSrc = R"(#version 330 core
        layout (location = 0) in vec3 a_Position;
        uniform mat4 u_ViewProjection;
        void main() {
            gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
        }
    )";

    const std::string fragmentSrc = R"(#version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(0.0, 1.0, 0.0, 1.0);
        }
    )";

    shader_ = std::make_shared<Shader>(vertexSrc, fragmentSrc);

    // Pre-allocate reusable VBO/VAO to avoid per-frame allocations
    // Initial capacity: kInitialLineCapacity lines * 2 vertices * 3 floats * 4 bytes
    bufferCapacity_ = kInitialLineCapacity * 2 * 3 * sizeof(float);
    vertex_buffer_  = std::make_shared<VertexBuffer>(bufferCapacity_);
    BufferLayout layout = {{ShaderDataType::Float3, "a_Position"}};
    vertex_buffer_->SetLayout(layout);

    vertex_array_ = std::make_shared<VertexArray>();
    vertex_array_->AddVertexBuffer(vertex_buffer_);

    lines_.reserve(kInitialLineCapacity);
    timedLines_.reserve(64);
}

PhysicsDebugDraw::~PhysicsDebugDraw() {}

void PhysicsDebugDraw::drawLine(const btVector3& from, const btVector3& to,
                                const btVector3& color) {
    std::lock_guard<std::mutex> lock(mutex_);
    lines_.push_back({Vector3(from.x(), from.y(), from.z()), Vector3(to.x(), to.y(), to.z()),
                      Vector3(color.x(), color.y(), color.z())});
}

void PhysicsDebugDraw::drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB,
                                        btScalar distance, int lifeTime, const btVector3& color) {
    // Draw a small cross at the contact point
    const float size = 0.05f;
    Vector3 point(PointOnB.x(), PointOnB.y(), PointOnB.z());
    Vector3 c(color.x(), color.y(), color.z());
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Cross in XYZ axes
    lines_.push_back({point - Vector3(size, 0, 0), point + Vector3(size, 0, 0), c});
    lines_.push_back({point - Vector3(0, size, 0), point + Vector3(0, size, 0), c});
    lines_.push_back({point - Vector3(0, 0, size), point + Vector3(0, 0, size), c});
    
    // Draw normal direction (in cyan)
    Vector3 normalEnd = point + Vector3(normalOnB.x(), normalOnB.y(), normalOnB.z()) * 0.2f;
    lines_.push_back({point, normalEnd, Vector3(0.0f, 1.0f, 1.0f)});
}

void PhysicsDebugDraw::reportErrorWarning(const char* warningString) {
    SE_LOG_WARN("Bullet Warning: {}", warningString);
}

void PhysicsDebugDraw::draw3dText(const btVector3& location, const char* textString) {}

void PhysicsDebugDraw::setDebugMode(int debugMode) {
    debug_mode_ = debugMode;
}

int PhysicsDebugDraw::getDebugMode() const {
    return debug_mode_;
}

void PhysicsDebugDraw::SetDebugDrawEnabled(bool enabled) {
    if (enabled) {
        SetMode(currentMode_ != DebugDrawMode::None ? currentMode_ : DebugDrawMode::Wireframe);
    } else {
        debug_mode_ = DBG_NoDebug;
    }
}

bool PhysicsDebugDraw::IsDebugDrawEnabled() const {
    return debug_mode_ != DBG_NoDebug;
}

void PhysicsDebugDraw::SetMode(DebugDrawMode mode) {
    currentMode_ = mode;
    switch (mode) {
        case DebugDrawMode::None:
            debug_mode_ = DBG_NoDebug;
            break;
        case DebugDrawMode::Wireframe:
            debug_mode_ = DBG_DrawWireframe;
            break;
        case DebugDrawMode::Aabb:
            debug_mode_ = DBG_DrawAabb;
            break;
        case DebugDrawMode::ContactPoints:
            debug_mode_ = DBG_DrawContactPoints;
            break;
        case DebugDrawMode::All:
            debug_mode_ = DBG_DrawWireframe | DBG_DrawAabb | DBG_DrawContactPoints;
            break;
    }
}

PhysicsDebugDraw::DebugDrawMode PhysicsDebugDraw::GetMode() const {
    return currentMode_;
}

void PhysicsDebugDraw::Flush(const Camera& camera) {
    for (const auto& timedLine : timedLines_) {
        // timedLines is main thread only - add to lines_ which is thread-safe
        drawLine(btVector3(timedLine.From.x, timedLine.From.y, timedLine.From.z),
                 btVector3(timedLine.To.x, timedLine.To.y, timedLine.To.z),
                 btVector3(timedLine.Color.x, timedLine.Color.y, timedLine.Color.z));
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (lines_.empty()) { return; }

    std::vector<float> vertices;
    vertices.reserve(lines_.size() * 6);  // 2 points * 3 floats

    for (const auto& line : lines_) {
        vertices.push_back(line.From.x);
        vertices.push_back(line.From.y);
        vertices.push_back(line.From.z);

        vertices.push_back(line.To.x);
        vertices.push_back(line.To.y);
        vertices.push_back(line.To.z);
    }

    const uint32_t requiredSize = static_cast<uint32_t>(vertices.size() * sizeof(float));

    // Reallocate buffer if capacity is exceeded (grow by 1.5x)
    if (requiredSize > bufferCapacity_) {
        bufferCapacity_ = static_cast<uint32_t>(requiredSize * 1.5f);
        vertex_buffer_  = std::make_shared<VertexBuffer>(bufferCapacity_);
        BufferLayout layout = {{ShaderDataType::Float3, "a_Position"}};
        vertex_buffer_->SetLayout(layout);

        vertex_array_ = std::make_shared<VertexArray>();
        vertex_array_->AddVertexBuffer(vertex_buffer_);
    }

    // Update existing buffer with new data
    vertex_buffer_->SetData(vertices.data(), requiredSize);

    if (shader_) {
        shader_->bind();
        auto& window      = Application::Get().GetWindow();
        float aspectRatio = (float)window.GetWidth() / (float)window.GetHeight();

        glm::mat4 view           = camera.getViewMatrix();
        glm::mat4 projection     = camera.getProjectionMatrix(aspectRatio);
        glm::mat4 viewProjection = projection * view;

        shader_->setMat4("u_ViewProjection", viewProjection);
    }

    RenderCommand::DrawLines(vertex_array_.get(), lines_.size() * 2);

    if (shader_) { shader_->unbind(); }
    lines_.clear();
}

void PhysicsDebugDraw::DrawDebugLine(const Vector3& from, const Vector3& to, const Vector3& color,
                                     float duration) {
    timedLines_.push_back({from, to, color, duration});
}

void PhysicsDebugDraw::DrawDebugSphere(const Vector3& center, float radius, const Vector3& color,
                                       float duration, int segments) {
    const float pi = 3.14159265359f;

    // Draw circles in XY, XZ, and YZ planes
    for (int i = 0; i < segments; ++i) {
        float angle1 = (float)i / segments * 2.0f * pi;
        float angle2 = (float)(i + 1) / segments * 2.0f * pi;

        // XZ circle (horizontal)
        Vector3 p1 = center + Vector3(cos(angle1) * radius, 0, sin(angle1) * radius);
        Vector3 p2 = center + Vector3(cos(angle2) * radius, 0, sin(angle2) * radius);
        timedLines_.push_back({p1, p2, color, duration});

        // XY circle (vertical front)
        p1 = center + Vector3(cos(angle1) * radius, sin(angle1) * radius, 0);
        p2 = center + Vector3(cos(angle2) * radius, sin(angle2) * radius, 0);
        timedLines_.push_back({p1, p2, color, duration});

        // YZ circle (vertical side)
        p1 = center + Vector3(0, sin(angle1) * radius, cos(angle1) * radius);
        p2 = center + Vector3(0, sin(angle2) * radius, cos(angle2) * radius);
        timedLines_.push_back({p1, p2, color, duration});
    }
}

void PhysicsDebugDraw::DrawDebugPoint(const Vector3& point, float size, const Vector3& color,
                                      float duration) {
    float half = size * 0.5f;
    // Draw a cross at the point
    timedLines_.push_back(
        {point - Vector3(half, 0, 0), point + Vector3(half, 0, 0), color, duration});
    timedLines_.push_back(
        {point - Vector3(0, half, 0), point + Vector3(0, half, 0), color, duration});
    timedLines_.push_back(
        {point - Vector3(0, 0, half), point + Vector3(0, 0, half), color, duration});
}

void PhysicsDebugDraw::UpdateTimedElements(float deltaTime) {
    // Remove expired timed lines
    timedLines_.erase(std::remove_if(timedLines_.begin(), timedLines_.end(),
                                     [deltaTime](TimedDebugLine& line) {
                                         line.RemainingTime -= deltaTime;
                                         return line.RemainingTime <= 0.0f;
                                     }),
                      timedLines_.end());
}

}  // namespace se
