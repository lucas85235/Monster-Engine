#pragma once

#include <string>
#include <glm.hpp>

namespace se {

class ComputeShader {
public:
    ComputeShader() = default;
    explicit ComputeShader(const std::string& computeSource);
    ~ComputeShader();
    
    ComputeShader(const ComputeShader&) = delete;
    ComputeShader& operator=(const ComputeShader&) = delete;
    
    ComputeShader(ComputeShader&& other) noexcept;
    ComputeShader& operator=(ComputeShader&& other) noexcept;
    
    void Bind() const;
    void Unbind() const;
    
    void Dispatch(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ) const;
    void DispatchAndWait(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ) const;
    
    // Uniforms
    void SetInt(const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetVec2(const std::string& name, const glm::vec2& value) const;
    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetVec4(const std::string& name, const glm::vec4& value) const;
    void SetMat4(const std::string& name, const glm::mat4& value) const;
    
    // For binding images
    void BindImageTexture(uint32_t unit, uint32_t texture, uint32_t access, uint32_t format) const;
    
    [[nodiscard]] uint32_t GetID() const { return programId_; }
    [[nodiscard]] bool IsValid() const { return programId_ != 0; }
    
    static std::shared_ptr<ComputeShader> CreateFromFile(const std::string& path);

private:
    uint32_t programId_ = 0;
    
    int GetUniformLocation(const std::string& name) const;
};

}  // namespace se
