#include "editor/EditorGrid.h"

#include <filesystem>
#include <gtc/type_ptr.hpp>

#include "engine/Log.h"
#include "engine/resources/MaterialManager.h"

namespace mst {

namespace fs = std::filesystem;

EditorGrid::EditorGrid() {
    LoadShader();
    CreateGridMesh();
}

EditorGrid::~EditorGrid() {
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (ebo_) glDeleteBuffers(1, &ebo_);
}

void EditorGrid::LoadShader() {
    fs::path assetsPath = fs::current_path() / "assets";
    fs::path vertPath = assetsPath / "shaders" / "grid.vert";
    fs::path fragPath = assetsPath / "shaders" / "grid.frag";

    if (!fs::exists(vertPath) || !fs::exists(fragPath)) {
        SE_LOG_ERROR("EditorGrid: Grid shaders not found at {}", vertPath.string());
        return;
    }

    shader_ = se::MaterialManager::GetShader("EditorGridShader", vertPath, fragPath);
    if (shader_) {
        SE_LOG_INFO("EditorGrid: Loaded grid shader");
    } else {
        SE_LOG_ERROR("EditorGrid: Failed to load grid shader");
    }
}

void EditorGrid::CreateGridMesh() {
    // Create a large quad on the Y=0 plane
    const float size = 100.0f;
    
    float vertices[] = {
        // Position (x, y, z)
        -size, 0.0f, -size,
         size, 0.0f, -size,
         size, 0.0f,  size,
        -size, 0.0f,  size
    };

    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    indexCount_ = 6;

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    SE_LOG_INFO("EditorGrid: Created grid mesh");
}

void EditorGrid::Render(const glm::mat4& view, const glm::mat4& projection) {
    if (!shader_ || !vao_) return;

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Enable depth testing - grid will be occluded by objects in front of it
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    // Keep depth writing enabled so grid properly occludes/is occluded
    glDepthMask(GL_TRUE);

    shader_->bind();
    shader_->setMat4("uView", view);
    shader_->setMat4("uProj", projection);
    shader_->setFloat("uGridSize", gridSize_);
    shader_->setFloat("uGridFade", fadeDistance_);
    shader_->setVec3("uGridColor", gridColor_);
    shader_->setVec3("uAxisXColor", axisXColor_);
    shader_->setVec3("uAxisZColor", axisZColor_);

    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Restore state
    glDisable(GL_BLEND);
}

}  // namespace mst
