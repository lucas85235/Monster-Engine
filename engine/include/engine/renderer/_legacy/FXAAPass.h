#pragma once
/**
 * FXAAPass.h - Fast Approximate Anti-Aliasing post-process
 */

#include "engine/renderer/PostProcessPass.h"
#include <memory>

namespace se {

class Shader;

class FXAAPass : public PostProcessPass {
public:
    FXAAPass() = default;
    ~FXAAPass() override = default;
    
    void Init(uint32_t width, uint32_t height) override;
    void Shutdown() override;
    void Resize(uint32_t width, uint32_t height) override;
    void Execute(GLuint inputTexture, GLuint outputFBO) override;
    void RenderUI() override;
    const char* GetName() const override { return "FXAA"; }

private:
    void RenderQuad();
    
    std::shared_ptr<Shader> shader_;
    GLuint quadVAO_ = 0;
    GLuint quadVBO_ = 0;
};

} // namespace se
