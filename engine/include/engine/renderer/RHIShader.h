#pragma once

#include <glm.hpp>
#include <memory>
#include <string>
#include <vector>

#include "engine/rhi/rhi_types.h"

namespace se {

class RHIShader {
   public:
    RHIShader(const std::string& vertPath, const std::string& fragPath);
    ~RHIShader();

    RHI::ShaderHandle GetHandle() const {
        return handle_;
    }

    void Bind() const;  // Note: In RHI, you typically bind a pipeline, not just a shader.
                        // This might need to create a default pipeline or be used differently.
                        // For now, it might be a no-op or throw if used incorrectly without a pipeline.

    // Uniforms (RHI abstraction)
    void SetFloat(const std::string& name, float value);
    void SetInt(const std::string& name, int value);
    void SetMat4(const std::string& name, const glm::mat4& value);
    void SetVec3(const std::string& name, const glm::vec3& value);
    void SetVec4(const std::string& name, const glm::vec4& value);

    static std::shared_ptr<RHIShader> Create(const std::string& vertPath, const std::string& fragPath);

   private:
    RHI::ShaderHandle handle_ = {0};
};

}  // namespace se
