#pragma once

#include <glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace RHI {
class ITexture;
}

#include "engine/renderer/Buffer.h"
#include "engine/renderer/Shader.h"
#include "engine/rhi/rhi_types.h"

namespace se {

class Material {
   public:
    Material(const std::shared_ptr<Shader>& shader);

    void Bind() const;
    void Unbind() const;

    void SetFloat(const std::string& name, float value);
    void SetInt(const std::string& name, int value);
    void SetVector3(const std::string& name, const Vector3& value);
    void SetVector4(const std::string& name, const Vector4& value);
    void SetMatrix4(const std::string& name, const Matrix4& value);

    std::shared_ptr<Shader> GetShader() const {
        return shader_;
    }

    RHI::PipelineHandle GetPipeline(const BufferLayout& layout);

   private:
    RHI::PipelineHandle pipeline_ = {0};

   private:
    std::shared_ptr<Shader> shader_;
    // Buffer storage for UBO data (CPU side)
    // Key: Binding/Set, Value: Vector of bytes
    std::unordered_map<uint32_t, std::vector<uint8_t>>              uniformBuffers_;
    std::unordered_map<std::string, std::shared_ptr<RHI::ITexture>> textures_;

    void InvalidUniform(const std::string& name);
};

}  // namespace se