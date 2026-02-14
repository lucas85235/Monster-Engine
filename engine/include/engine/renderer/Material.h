#pragma once

#include <glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>

#include "engine/Shader.h"

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

   private:
    std::shared_ptr<Shader>                    shader_;
    std::unordered_map<std::string, float>     floatUniforms_;
    std::unordered_map<std::string, int>       intUniforms_;
    std::unordered_map<std::string, Vector3> vec3Uniforms_;
    std::unordered_map<std::string, Vector4> vec4Uniforms_;
    std::unordered_map<std::string, Matrix4> mat4Uniforms_;
};

}  // namespace se