#pragma once

#include "engine/renderer/PostProcessPass.h"
#include "engine/Shader.h"

#include <memory>
#include <vector>
#include <glm.hpp>

namespace se {

class SSAOPass : public PostProcessPass {
public:
    SSAOPass() = default;
    ~SSAOPass() override = default;

    void Init(uint32_t width, uint32_t height) override;
    void Shutdown() override;
    void Resize(uint32_t width, uint32_t height) override;
    void Execute(GLuint inputTexture, GLuint outputFBO) override;
    void RenderUI() override;
    const char* GetName() const override { return "SSAO"; }

    void SetDepthTexture(GLuint depthTex) { depthTexture_ = depthTex; }
    void SetNormalTexture(GLuint normalTex) { normalTexture_ = normalTex; }
    void SetProjectionMatrix(const glm::mat4& proj) { projection_ = proj; }
    void SetViewMatrix(const glm::mat4& view) { view_ = view; }

    GLuint GetAOTexture() const { return blurredAOTexture_; }

    float GetRadius() const { return radius_; }
    void SetRadius(float r) { radius_ = r; }
    float GetIntensity() const { return intensity_; }
    void SetIntensity(float i) { intensity_ = i; }

private:
    void GenerateKernel();
    void CreateNoiseTexture();
    void CreateFramebuffers();
    void DestroyFramebuffers();
    void RenderQuad();

    std::shared_ptr<Shader> ssaoShader_;
    std::shared_ptr<Shader> blurShader_;

    GLuint ssaoFBO_ = 0;
    GLuint aoTexture_ = 0;
    GLuint blurFBO_ = 0;
    GLuint blurredAOTexture_ = 0;
    GLuint noiseTexture_ = 0;

    GLuint depthTexture_ = 0;
    GLuint normalTexture_ = 0;

    std::vector<glm::vec3> ssaoKernel_;
    
    glm::mat4 projection_{1.0f};
    glm::mat4 view_{1.0f};

    float radius_ = 0.5f;
    float intensity_ = 1.5f;
    float bias_ = 0.025f;
    int kernelSize_ = 32;
    int blurSize_ = 4;
    
    // Cached kernel uniform locations (avoid string allocation per frame)
    std::vector<int> kernelLocations_;

    GLuint quadVAO_ = 0;
    GLuint quadVBO_ = 0;
};

}  // namespace se
