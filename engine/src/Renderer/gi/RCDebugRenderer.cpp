#include "engine/renderer/gi/RCDebugRenderer.h"
#include "engine/renderer/gi/SparseBrickCache.h"
#include "engine/Shader.h"
#include "engine/Log.h"

#include <glad/glad.h>
#include <gtc/matrix_transform.hpp>


namespace se {
namespace gi {

namespace {

const char* kWireframeVertexShader = R"(#version 430 core
layout(location = 0) in vec3 a_Position;

uniform mat4 u_ViewProj;
uniform mat4 u_Model;
uniform vec3 u_Color;

out vec3 v_Color;

void main() {
    gl_Position = u_ViewProj * u_Model * vec4(a_Position, 1.0);
    v_Color = u_Color;
}
)";

const char* kWireframeFragmentShader = R"(#version 430 core
in vec3 v_Color;
out vec4 FragColor;

void main() {
    FragColor = vec4(v_Color, 1.0);
}
)";

const char* kOverlayVertexShader = R"(#version 430 core
layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TexCoord;

out vec2 v_TexCoord;

void main() {
    gl_Position = vec4(a_Position, 0.0, 1.0);
    v_TexCoord = a_TexCoord;
}
)";

const char* kOverlayFragmentShader = R"(#version 430 core
in vec2 v_TexCoord;
out vec4 FragColor;

uniform sampler2D u_Texture;
uniform float u_Opacity;

void main() {
    vec4 color = texture(u_Texture, v_TexCoord);
    FragColor = vec4(color.rgb, color.a * u_Opacity);
}
)";

const float kCubeVertices[] = {
    0,0,0, 1,0,0,  1,0,0, 1,1,0,  1,1,0, 0,1,0,  0,1,0, 0,0,0,
    0,0,1, 1,0,1,  1,0,1, 1,1,1,  1,1,1, 0,1,1,  0,1,1, 0,0,1,
    0,0,0, 0,0,1,  1,0,0, 1,0,1,  1,1,0, 1,1,1,  0,1,0, 0,1,1
};

const float kQuadVertices[] = {
    -1, -1, 0, 0,
     1, -1, 1, 0,
     1,  1, 1, 1,
    -1, -1, 0, 0,
     1,  1, 1, 1,
    -1,  1, 0, 1
};

}  // namespace

RCDebugRenderer::RCDebugRenderer() = default;

RCDebugRenderer::~RCDebugRenderer() {
    Shutdown();
}

void RCDebugRenderer::Init() {
    if (initialized_) return;
    
    CreateResources();
    initialized_ = true;
    
    SE_LOG_INFO("RCDebugRenderer initialized");
}

void RCDebugRenderer::Shutdown() {
    if (!initialized_) return;
    
    DestroyResources();
    initialized_ = false;
}

void RCDebugRenderer::CreateResources() {
    wireframeShader_ = std::make_shared<Shader>(kWireframeVertexShader, kWireframeFragmentShader);
    
    overlayShader_ = std::make_shared<Shader>(kOverlayVertexShader, kOverlayFragmentShader);

    
    glGenVertexArrays(1, &cubeVAO_);
    glGenBuffers(1, &cubeVBO_);
    
    glBindVertexArray(cubeVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kCubeVertices), kCubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    
    glGenVertexArrays(1, &quadVAO_);
    glGenBuffers(1, &quadVBO_);
    
    glBindVertexArray(quadVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuadVertices), kQuadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

void RCDebugRenderer::DestroyResources() {
    wireframeShader_.reset();
    overlayShader_.reset();
    sliceShader_.reset();
    
    if (cubeVAO_) { glDeleteVertexArrays(1, &cubeVAO_); cubeVAO_ = 0; }
    if (cubeVBO_) { glDeleteBuffers(1, &cubeVBO_); cubeVBO_ = 0; }
    if (quadVAO_) { glDeleteVertexArrays(1, &quadVAO_); quadVAO_ = 0; }
    if (quadVBO_) { glDeleteBuffers(1, &quadVBO_); quadVBO_ = 0; }
}

glm::vec3 RCDebugRenderer::GetCascadeColor(int cascadeLevel) const {
    static const glm::vec3 colors[] = {
        {1.0f, 0.2f, 0.2f},
        {0.2f, 1.0f, 0.2f},
        {0.2f, 0.2f, 1.0f},
        {1.0f, 1.0f, 0.2f},
        {1.0f, 0.2f, 1.0f},
        {0.2f, 1.0f, 1.0f},
        {1.0f, 0.6f, 0.2f},
        {0.6f, 0.2f, 1.0f}
    };
    return colors[cascadeLevel % 8];
}

void RCDebugRenderer::Render(const glm::mat4& viewProj,
                              const SparseBrickCache* brickCache,
                              uint32_t radianceTexture) {
    if (!initialized_) return;
    
    switch (config_.mode) {
        case RCDebugMode::ActiveBricks:
            RenderActiveBricks(viewProj, brickCache);
            break;
        case RCDebugMode::CascadeLevels:
            RenderCascadeLevels(viewProj, brickCache);
            break;
        case RCDebugMode::RadianceSlice:
            RenderRadianceSlice(viewProj, radianceTexture, config_.sliceAxis, config_.slicePosition);
            break;
        default:
            break;
    }
}

void RCDebugRenderer::RenderActiveBricks(const glm::mat4& viewProj,
                                          const SparseBrickCache* brickCache) {
    if (!brickCache || !wireframeShader_) {
        SE_LOG_WARN("RenderActiveBricks: brickCache={}, shader={}", 
                    brickCache != nullptr, wireframeShader_ != nullptr);
        return;
    }
    
    wireframeShader_->bind();
    wireframeShader_->setMat4("u_ViewProj", viewProj);
    
    glBindVertexArray(cubeVAO_);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(2.0f);
    glDisable(GL_DEPTH_TEST);
    
    const auto& config = brickCache->GetConfig();
    int totalRendered = 0;
    
    for (int cascade = 0; cascade < config.numCascades; ++cascade) {
        std::vector<BrickKey> bricks;
        brickCache->GetActiveBricks(cascade, bricks);
        
        if (bricks.empty()) continue;
        
        glm::vec3 color = GetCascadeColor(cascade);
        wireframeShader_->setVec3("u_Color", color);
        
        float voxelSize = brickCache->GetCascadeVoxelSize(cascade);
        float brickWorldSize = voxelSize * config.brickSize;
        
        for (const auto& key : bricks) {
            glm::vec3 worldPos = brickCache->BrickKeyToWorldPos(key);
            
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, worldPos - glm::vec3(brickWorldSize * 0.5f));
            model = glm::scale(model, glm::vec3(brickWorldSize));
            
            wireframeShader_->setMat4("u_Model", model);
            glDrawArrays(GL_LINES, 0, 24);
            totalRendered++;
        }
    }
    
    SE_LOG_INFO("RenderActiveBricks: rendered {} brick wireframes", totalRendered);
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
}


void RCDebugRenderer::RenderRadianceSlice(const glm::mat4& viewProj,
                                           uint32_t radianceTexture,
                                           int axis, float position) {
    if (!overlayShader_ || radianceTexture == 0) return;
    
    overlayShader_->bind();
    overlayShader_->setFloat("u_Opacity", config_.opacity);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, radianceTexture);
    overlayShader_->setInt("u_Texture", 0);
    
    glBindVertexArray(quadVAO_);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
}

void RCDebugRenderer::RenderIrradianceOverlay(uint32_t irradianceTexture,
                                               int screenWidth, int screenHeight) {
    if (!overlayShader_ || irradianceTexture == 0) return;
    
    overlayShader_->bind();
    overlayShader_->setFloat("u_Opacity", config_.opacity);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, irradianceTexture);
    overlayShader_->setInt("u_Texture", 0);
    
    glBindVertexArray(quadVAO_);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
}

void RCDebugRenderer::RenderCascadeLevels(const glm::mat4& viewProj,
                                           const SparseBrickCache* brickCache) {
    RenderActiveBricks(viewProj, brickCache);
}

}  // namespace gi
}  // namespace se
