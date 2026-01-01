#pragma once

#include "engine/renderer/PostProcessPass.h"
#include "engine/Shader.h"

#include <memory>

namespace se {

enum class TonemapOperator {
    ACES = 0,
    Reinhard,
    Uncharted2,
    Neutral,
    AgX
};

class TonemappingPass : public PostProcessPass {
public:
    TonemappingPass() = default;
    ~TonemappingPass() override = default;

    void Init(uint32_t width, uint32_t height) override;
    void Shutdown() override;
    void Resize(uint32_t width, uint32_t height) override;
    void Execute(GLuint inputTexture, GLuint outputFBO) override;
    void RenderUI() override;
    const char* GetName() const override { return "Tonemapping"; }

    float GetExposure() const { return exposure_; }
    void SetExposure(float e) { exposure_ = e; }
    float GetGamma() const { return gamma_; }
    void SetGamma(float g) { gamma_ = g; }
    TonemapOperator GetOperator() const { return tonemapOp_; }
    void SetOperator(TonemapOperator op) { tonemapOp_ = op; }

private:
    void RenderQuad();

    std::shared_ptr<Shader> tonemapShader_;

    float exposure_ = 1.0f;
    float gamma_ = 2.2f;
    TonemapOperator tonemapOp_ = TonemapOperator::ACES;

    GLuint quadVAO_ = 0;
    GLuint quadVBO_ = 0;
};

}  // namespace se
