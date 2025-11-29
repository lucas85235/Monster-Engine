#include "engine/physics/PhysicsDebugDraw.h"
#include <glad/glad.h>
#include <iostream>
#include "engine/Application.h"
#include "engine/Log.h"

namespace se {

PhysicsDebugDraw::PhysicsDebugDraw() {
    debug_mode_ = DBG_DrawWireframe;

    const std::string vertexSrc = R"(
        #version 330 core
        layout (location = 0) in vec3 a_Position;
        uniform mat4 u_ViewProjection;
        void main() {
            gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
        }
    )";

    const std::string fragmentSrc = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(0.0, 1.0, 0.0, 1.0);
        }
    )";

    shader_ = std::make_shared<Shader>(vertexSrc, fragmentSrc);
}

PhysicsDebugDraw::~PhysicsDebugDraw() {}

void PhysicsDebugDraw::drawLine(const btVector3& from, const btVector3& to, const btVector3& color) {
    lines_.push_back({
        Vector3(from.x(), from.y(), from.z()),
        Vector3(to.x(), to.y(), to.z()),
        Vector3(color.x(), color.y(), color.z())
    });
}

void PhysicsDebugDraw::drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) {
}

void PhysicsDebugDraw::reportErrorWarning(const char* warningString) {
    SE_LOG_WARN("Bullet Warning: {}", warningString);
}

void PhysicsDebugDraw::draw3dText(const btVector3& location, const char* textString) {
}

void PhysicsDebugDraw::setDebugMode(int debugMode) {
    debug_mode_ = debugMode;
}

int PhysicsDebugDraw::getDebugMode() const {
    return debug_mode_;
}

void PhysicsDebugDraw::Flush(const Camera& camera) {
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

    auto vb = std::make_shared<VertexBuffer>(vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(float)));
    BufferLayout layout = { {ShaderDataType::Float3, "a_Position"} };
    vb->SetLayout(layout);

    auto va = std::make_shared<VertexArray>();
    va->AddVertexBuffer(vb);

    if (shader_) {
        shader_->bind();
        // Camera doesn't have a stored aspect ratio, we need to get it from window or pass it in.
        // For now, let's assume a standard aspect ratio or get it from Application.
        auto& window = Application::Get().GetWindow();
        float aspectRatio = (float)window.GetWidth() / (float)window.GetHeight();
        
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);
        glm::mat4 viewProjection = projection * view;
        
        shader_->setMat4("u_ViewProjection", viewProjection);
    }

    RenderCommand::DrawLines(va.get(), lines_.size() * 2);

    if (shader_) {
        shader_->unbind();
    }
    lines_.clear();
}

} // namespace se
