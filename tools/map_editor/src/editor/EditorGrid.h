#pragma once
/**
 * EditorGrid.h - Infinite ground grid rendered via fragment shader.
 *
 * Displays grid lines with fade-out at distance and axis highlighting.
 * Grid renders on Y=0 plane with configurable size and colors.
 */

#include <memory>
#include <glad/glad.h>
#include <glm.hpp>

#include "engine/Shader.h"

namespace mst {

class EditorGrid {
   public:
    EditorGrid();
    ~EditorGrid();

    void Render(const glm::mat4& view, const glm::mat4& projection);

    void SetGridSize(float size) { gridSize_ = size; }
    void SetFadeDistance(float dist) { fadeDistance_ = dist; }
    void SetGridColor(const glm::vec3& color) { gridColor_ = color; }
    void SetAxisColors(const glm::vec3& xColor, const glm::vec3& zColor) {
        axisXColor_ = xColor;
        axisZColor_ = zColor;
    }

   private:
    void CreateGridMesh();
    void LoadShader();

    std::shared_ptr<se::Shader> shader_;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint ebo_ = 0;
    int indexCount_ = 0;

    float gridSize_ = 1.0f;
    float fadeDistance_ = 50.0f;
    glm::vec3 gridColor_{0.5f, 0.5f, 0.5f};
    glm::vec3 axisXColor_{0.8f, 0.2f, 0.2f};
    glm::vec3 axisZColor_{0.2f, 0.2f, 0.8f};
};

}  // namespace mst
