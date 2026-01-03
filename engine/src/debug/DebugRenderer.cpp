#include "engine/debug/DebugRenderer.h"
#include "engine/Application.h"
#include "engine/Camera.h"
#include "engine/Shader.h"
#include "engine/renderer/RenderCommand.h"
#include "engine/renderer/VertexArray.h"
#include "engine/Log.h"

#include <glad/glad.h>
#include <algorithm>
#include <cmath>

namespace se {

DebugRenderer& DebugRenderer::Get() {
    static DebugRenderer instance;
    return instance;
}

DebugRenderer::DebugRenderer() {
    lines_.reserve(kInitialLineCapacity);
    persistentLines_.reserve(256);
}

void DebugRenderer::InitializeResources() {
    if (initialized_) return;

    const std::string vertexSrc = R"(#version 330 core
        layout (location = 0) in vec3 a_Position;
        layout (location = 1) in vec3 a_Color;
        
        uniform mat4 u_ViewProjection;
        
        out vec3 v_Color;
        
        void main() {
            gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
            v_Color = a_Color;
        }
    )";

    const std::string fragmentSrc = R"(#version 330 core
        in vec3 v_Color;
        out vec4 FragColor;
        
        void main() {
            FragColor = vec4(v_Color, 1.0);
        }
    )";

    shader_ = std::make_shared<Shader>(vertexSrc, fragmentSrc);

    // Pre-allocate buffer: capacity lines * 2 vertices * (3 pos + 3 color) floats
    bufferCapacity_ = kInitialLineCapacity * 2 * 6 * sizeof(float);
    vertexBuffer_ = std::make_shared<VertexBuffer>(bufferCapacity_);
    
    BufferLayout layout = {
        {ShaderDataType::Float3, "a_Position"},
        {ShaderDataType::Float3, "a_Color"}
    };
    vertexBuffer_->SetLayout(layout);

    vertexArray_ = std::make_shared<VertexArray>();
    vertexArray_->AddVertexBuffer(vertexBuffer_);

    initialized_ = true;
    SE_LOG_DEBUG("[DebugRenderer] Initialized with capacity for {} lines", kInitialLineCapacity);
}

void DebugRenderer::DrawLine(const Vector3& from, const Vector3& to, const Vector3& color) {
    if (!enabled_) return;
    std::lock_guard<std::mutex> lock(mutex_);
    lines_.push_back({from, to, color});
}

void DebugRenderer::DrawSphere(const Vector3& center, float radius, const Vector3& color, int segments) {
    if (!enabled_) return;
    
    const float pi = 3.14159265359f;
    
    for (int i = 0; i < segments; ++i) {
        float angle1 = (float)i / segments * 2.0f * pi;
        float angle2 = (float)(i + 1) / segments * 2.0f * pi;

        // XZ circle (horizontal)
        Vector3 p1 = center + Vector3(std::cos(angle1) * radius, 0, std::sin(angle1) * radius);
        Vector3 p2 = center + Vector3(std::cos(angle2) * radius, 0, std::sin(angle2) * radius);
        DrawLine(p1, p2, color);

        // XY circle (vertical front)
        p1 = center + Vector3(std::cos(angle1) * radius, std::sin(angle1) * radius, 0);
        p2 = center + Vector3(std::cos(angle2) * radius, std::sin(angle2) * radius, 0);
        DrawLine(p1, p2, color);

        // YZ circle (vertical side)
        p1 = center + Vector3(0, std::sin(angle1) * radius, std::cos(angle1) * radius);
        p2 = center + Vector3(0, std::sin(angle2) * radius, std::cos(angle2) * radius);
        DrawLine(p1, p2, color);
    }
}

void DebugRenderer::DrawBox(const Vector3& min, const Vector3& max, const Vector3& color) {
    if (!enabled_) return;
    
    // Bottom face
    DrawLine({min.x, min.y, min.z}, {max.x, min.y, min.z}, color);
    DrawLine({max.x, min.y, min.z}, {max.x, min.y, max.z}, color);
    DrawLine({max.x, min.y, max.z}, {min.x, min.y, max.z}, color);
    DrawLine({min.x, min.y, max.z}, {min.x, min.y, min.z}, color);
    
    // Top face
    DrawLine({min.x, max.y, min.z}, {max.x, max.y, min.z}, color);
    DrawLine({max.x, max.y, min.z}, {max.x, max.y, max.z}, color);
    DrawLine({max.x, max.y, max.z}, {min.x, max.y, max.z}, color);
    DrawLine({min.x, max.y, max.z}, {min.x, max.y, min.z}, color);
    
    // Vertical edges
    DrawLine({min.x, min.y, min.z}, {min.x, max.y, min.z}, color);
    DrawLine({max.x, min.y, min.z}, {max.x, max.y, min.z}, color);
    DrawLine({max.x, min.y, max.z}, {max.x, max.y, max.z}, color);
    DrawLine({min.x, min.y, max.z}, {min.x, max.y, max.z}, color);
}

void DebugRenderer::DrawPoint(const Vector3& pos, float size, const Vector3& color) {
    if (!enabled_) return;
    
    float half = size * 0.5f;
    DrawLine(pos - Vector3(half, 0, 0), pos + Vector3(half, 0, 0), color);
    DrawLine(pos - Vector3(0, half, 0), pos + Vector3(0, half, 0), color);
    DrawLine(pos - Vector3(0, 0, half), pos + Vector3(0, 0, half), color);
}

void DebugRenderer::DrawArrow(const Vector3& from, const Vector3& to, const Vector3& color, float headSize) {
    if (!enabled_) return;
    
    DrawLine(from, to, color);
    
    // Arrow head
    Vector3 dir = glm::normalize(to - from);
    Vector3 right = glm::normalize(glm::cross(dir, Vector3(0, 1, 0)));
    if (glm::length(right) < 0.001f) {
        right = glm::normalize(glm::cross(dir, Vector3(1, 0, 0)));
    }
    Vector3 up = glm::normalize(glm::cross(right, dir));
    
    Vector3 headBase = to - dir * headSize;
    DrawLine(to, headBase + right * headSize * 0.5f, color);
    DrawLine(to, headBase - right * headSize * 0.5f, color);
    DrawLine(to, headBase + up * headSize * 0.5f, color);
    DrawLine(to, headBase - up * headSize * 0.5f, color);
}

void DebugRenderer::DrawCircle(const Vector3& center, float radius, const Vector3& color,
                                const Vector3& normal, int segments) {
    if (!enabled_) return;
    
    // Find perpendicular vectors to the normal
    Vector3 right = glm::normalize(glm::cross(normal, Vector3(0, 1, 0)));
    if (glm::length(right) < 0.001f) {
        right = glm::normalize(glm::cross(normal, Vector3(1, 0, 0)));
    }
    Vector3 forward = glm::normalize(glm::cross(right, normal));
    
    const float pi = 3.14159265359f;
    for (int i = 0; i < segments; ++i) {
        float angle1 = (float)i / segments * 2.0f * pi;
        float angle2 = (float)(i + 1) / segments * 2.0f * pi;
        
        Vector3 p1 = center + right * std::cos(angle1) * radius + forward * std::sin(angle1) * radius;
        Vector3 p2 = center + right * std::cos(angle2) * radius + forward * std::sin(angle2) * radius;
        DrawLine(p1, p2, color);
    }
}

void DebugRenderer::DrawPath(const std::vector<Vector3>& waypoints, const Vector3& color, float waypointSize) {
    if (!enabled_ || waypoints.empty()) return;
    
    // Draw lines between waypoints
    for (size_t i = 0; i < waypoints.size() - 1; ++i) {
        DrawLine(waypoints[i], waypoints[i + 1], color);
    }
    
    // Draw spheres at each waypoint
    for (const auto& wp : waypoints) {
        DrawSphere(wp, waypointSize, color, 6);
    }
}

void DebugRenderer::DrawSquare(const Vector3& center, float halfSize, const Vector3& color,
                                const Vector3& normal) {
    if (!enabled_) return;
    
    Vector3 right = glm::normalize(glm::cross(normal, Vector3(0, 0, 1)));
    if (glm::length(right) < 0.001f) {
        right = glm::normalize(glm::cross(normal, Vector3(1, 0, 0)));
    }
    Vector3 forward = glm::normalize(glm::cross(right, normal));
    
    Vector3 corners[4] = {
        center + right * halfSize + forward * halfSize,
        center - right * halfSize + forward * halfSize,
        center - right * halfSize - forward * halfSize,
        center + right * halfSize - forward * halfSize
    };
    
    DrawLine(corners[0], corners[1], color);
    DrawLine(corners[1], corners[2], color);
    DrawLine(corners[2], corners[3], color);
    DrawLine(corners[3], corners[0], color);
}

void DebugRenderer::DrawPersistentLine(const Vector3& from, const Vector3& to, 
                                        const Vector3& color, float duration) {
    if (!enabled_) return;
    std::lock_guard<std::mutex> lock(mutex_);
    persistentLines_.push_back({from, to, color, duration});
}

void DebugRenderer::DrawPersistentSphere(const Vector3& center, float radius, 
                                          const Vector3& color, float duration, int segments) {
    if (!enabled_) return;
    
    const float pi = 3.14159265359f;
    
    for (int i = 0; i < segments; ++i) {
        float angle1 = (float)i / segments * 2.0f * pi;
        float angle2 = (float)(i + 1) / segments * 2.0f * pi;

        Vector3 p1 = center + Vector3(std::cos(angle1) * radius, 0, std::sin(angle1) * radius);
        Vector3 p2 = center + Vector3(std::cos(angle2) * radius, 0, std::sin(angle2) * radius);
        DrawPersistentLine(p1, p2, color, duration);

        p1 = center + Vector3(std::cos(angle1) * radius, std::sin(angle1) * radius, 0);
        p2 = center + Vector3(std::cos(angle2) * radius, std::sin(angle2) * radius, 0);
        DrawPersistentLine(p1, p2, color, duration);

        p1 = center + Vector3(0, std::sin(angle1) * radius, std::cos(angle1) * radius);
        p2 = center + Vector3(0, std::sin(angle2) * radius, std::cos(angle2) * radius);
        DrawPersistentLine(p1, p2, color, duration);
    }
}

void DebugRenderer::DrawPersistentPoint(const Vector3& pos, float size, 
                                         const Vector3& color, float duration) {
    if (!enabled_) return;
    
    float half = size * 0.5f;
    DrawPersistentLine(pos - Vector3(half, 0, 0), pos + Vector3(half, 0, 0), color, duration);
    DrawPersistentLine(pos - Vector3(0, half, 0), pos + Vector3(0, half, 0), color, duration);
    DrawPersistentLine(pos - Vector3(0, 0, half), pos + Vector3(0, 0, half), color, duration);
}

void DebugRenderer::Update(float deltaTime) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    persistentLines_.erase(
        std::remove_if(persistentLines_.begin(), persistentLines_.end(),
            [deltaTime](PersistentLine& line) {
                line.remainingTime -= deltaTime;
                return line.remainingTime <= 0.0f;
            }),
        persistentLines_.end()
    );
}

void DebugRenderer::Flush(const Camera& camera) {
    if (!enabled_) {
        lines_.clear();
        return;
    }
    
    InitializeResources();
    
    // Add persistent lines to immediate for rendering
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& pLine : persistentLines_) {
            lines_.push_back({pLine.from, pLine.to, pLine.color});
        }
    }
    
    if (lines_.empty()) return;
    
    // Build vertex data: position (3) + color (3) per vertex
    std::vector<float> vertices;
    vertices.reserve(lines_.size() * 12);  // 2 vertices * 6 floats
    
    for (const auto& line : lines_) {
        // From vertex
        vertices.push_back(line.from.x);
        vertices.push_back(line.from.y);
        vertices.push_back(line.from.z);
        vertices.push_back(line.color.x);
        vertices.push_back(line.color.y);
        vertices.push_back(line.color.z);
        
        // To vertex
        vertices.push_back(line.to.x);
        vertices.push_back(line.to.y);
        vertices.push_back(line.to.z);
        vertices.push_back(line.color.x);
        vertices.push_back(line.color.y);
        vertices.push_back(line.color.z);
    }
    
    const uint32_t requiredSize = static_cast<uint32_t>(vertices.size() * sizeof(float));
    
    // Grow buffer if needed
    if (requiredSize > bufferCapacity_) {
        bufferCapacity_ = static_cast<uint32_t>(requiredSize * 1.5f);
        vertexBuffer_ = std::make_shared<VertexBuffer>(bufferCapacity_);
        
        BufferLayout layout = {
            {ShaderDataType::Float3, "a_Position"},
            {ShaderDataType::Float3, "a_Color"}
        };
        vertexBuffer_->SetLayout(layout);
        
        vertexArray_ = std::make_shared<VertexArray>();
        vertexArray_->AddVertexBuffer(vertexBuffer_);
    }
    
    vertexBuffer_->SetData(vertices.data(), requiredSize);
    
    if (shader_) {
        shader_->bind();
        
        auto& window = Application::Get().GetWindow();
        float aspectRatio = (float)window.GetWidth() / (float)window.GetHeight();
        
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);
        glm::mat4 viewProjection = projection * view;
        
        shader_->setMat4("u_ViewProjection", viewProjection);
    }
    
    glLineWidth(1.0f);
    RenderCommand::DrawLines(vertexArray_.get(), static_cast<uint32_t>(lines_.size() * 2));
    
    if (shader_) {
        shader_->unbind();
    }
    
    lines_.clear();
}

void DebugRenderer::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    lines_.clear();
}

}  // namespace se
