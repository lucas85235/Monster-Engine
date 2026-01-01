#pragma once

#include <cstdint>
#include <glad/glad.h>
#include <string>

namespace se {

class PostProcessPass {
public:
    virtual ~PostProcessPass() = default;

    virtual void Init(uint32_t width, uint32_t height) = 0;
    virtual void Shutdown() = 0;
    virtual void Resize(uint32_t width, uint32_t height) = 0;
    virtual void Execute(GLuint inputTexture, GLuint outputFBO) = 0;
    virtual void RenderUI() {}
    virtual const char* GetName() const = 0;

    bool IsEnabled() const { return enabled_; }
    void SetEnabled(bool enabled) { enabled_ = enabled; }

protected:
    bool enabled_ = true;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
};

}  // namespace se
