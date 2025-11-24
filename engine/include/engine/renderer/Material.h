#pragma once

#include "engine/Shader.h"
#include <glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace se {

class Material {
  public:
    Material(const std::shared_ptr<Shader>& shader);

    void Bind() const;
    void Unbind() const;

    void SetFloat(const std::string& name, float value);
    void SetInt(const std::string& name, int value);
    void SetVector3(const std::string& name, const glm::vec3& value);
    void SetVector4(const std::string& name, const glm::vec4& value);
    void SetMatrix4(const std::string& name, const glm::mat4& value);

    std::shared_ptr<Shader> GetShader() const {
        return shader_;
    }

  private:
    std::shared_ptr<Shader> shader_;
    std::unordered_map<std::string, float> floatUniforms_;
    std::unordered_map<std::string, int> intUniforms_;
    std::unordered_map<std::string, glm::vec3> vec3Uniforms_;
    std::unordered_map<std::string, glm::vec4> vec4Uniforms_;
    std::unordered_map<std::string, glm::mat4> mat4Uniforms_;
};

} // namespace se