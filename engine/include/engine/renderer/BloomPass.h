#pragma once

#include "engine/renderer/PostProcessPass.h"
#include "engine/Shader.h"

#include <memory>
#include <vector>

namespace se {

class BloomPass : public PostProcessPass {
public:
    BloomPass() = default;
    ~BloomPass() override = default;

    void Init(uint32_t width, uint32_t height) override;
    void Shutdown() override;
    void Resize(uint32_t width, uint32_t height) override;
    void Execute(GLuint inputTexture, GLuint outputFBO) override;
    void RenderUI() override;
    const char* GetName() const override { return "Bloom"; }

    float GetThreshold() const { return threshold_; }
    void SetThreshold(float t) { threshold_ = t; }
    float GetIntensity() const { return intensity_; }
    void SetIntensity(float i) { intensity_ = i; }

private:
    void CreateMipChain();
    void DestroyMipChain();
    void RenderQuad();

    struct BloomMip {
        GLuint fbo = 0;
        GLuint texture = 0;
        uint32_t width = 0;
        uint32_t height = 0;
    };

    std::shared_ptr<Shader> thresholdShader_;
    std::shared_ptr<Shader> downsampleShader_;
    std::shared_ptr<Shader> upsampleShader_;
    std::shared_ptr<Shader> compositeShader_;

    std::vector<BloomMip> mipChain_;
    static constexpr int MIP_COUNT = 6;

    float threshold_ = 1.0f;
    float softKnee_ = 0.5f;
    float intensity_ = 0.5f;
    float filterRadius_ = 0.005f;

    GLuint quadVAO_ = 0;
    GLuint quadVBO_ = 0;
};

}  // namespace se
