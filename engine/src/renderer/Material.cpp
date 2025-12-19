#include "engine/renderer/Material.h"

namespace se {
Material::Material(const std::shared_ptr<Shader>& shader) : shader_(shader) {}

void Material::Bind() const {
    shader_->bind();

    // Apply all cached uniforms
    for (const auto& [name, value] : intUniforms_) { shader_->setInt(name.c_str(), value); }

    for (const auto& [name, value] : floatUniforms_) { shader_->setFloat(name.c_str(), value); }

    for (const auto& [name, value] : vec3Uniforms_) { shader_->setVec3(name.c_str(), value); }

    for (const auto& [name, value] : vec4Uniforms_) { shader_->setVec4(name.c_str(), value); }

    for (const auto& [name, value] : mat4Uniforms_) { shader_->setMat4(name.c_str(), value); }
}

void Material::Unbind() const {
    shader_->unbind();
}

void Material::SetFloat(const std::string& name, float value) {
    floatUniforms_[name] = value;
}

void Material::SetInt(const std::string& name, int value) {
    intUniforms_[name] = value;
}

void Material::SetVector3(const std::string& name, const Vector3& value) {
    vec3Uniforms_[name] = value;
}

void Material::SetVector4(const std::string& name, const Vector4& value) {
    vec4Uniforms_[name] = value;
}

void Material::SetMatrix4(const std::string& name, const Matrix4& value) {
    mat4Uniforms_[name] = value;
}
}  // namespace se
