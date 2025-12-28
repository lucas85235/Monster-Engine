#include "engine/renderer/ComputeShader.h"

#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <gtc/type_ptr.hpp>

#include "engine/Log.h"

namespace se {

ComputeShader::ComputeShader(const std::string& computeSource) {
    SE_LOG_INFO("Creating compute shader");
    
    // Compile compute shader
    GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
    const char* src = computeSource.c_str();
    glShaderSource(computeShader, 1, &src, nullptr);
    glCompileShader(computeShader);
    
    // Check compilation
    GLint success;
    glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(computeShader, 1024, nullptr, infoLog);
        SE_LOG_ERROR("Compute shader compilation failed: {}", infoLog);
        glDeleteShader(computeShader);
        return;
    }
    
    // Link program
    programId_ = glCreateProgram();
    glAttachShader(programId_, computeShader);
    glLinkProgram(programId_);
    
    glGetProgramiv(programId_, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(programId_, 1024, nullptr, infoLog);
        SE_LOG_ERROR("Compute shader linking failed: {}", infoLog);
        glDeleteProgram(programId_);
        programId_ = 0;
    }
    
    glDeleteShader(computeShader);
    
    if (programId_ != 0) {
        SE_LOG_INFO("Compute shader created successfully (ID: {})", programId_);
    }
}

ComputeShader::~ComputeShader() {
    if (programId_ != 0) {
        glDeleteProgram(programId_);
        programId_ = 0;
    }
}

ComputeShader::ComputeShader(ComputeShader&& other) noexcept
    : programId_(other.programId_) {
    other.programId_ = 0;
}

ComputeShader& ComputeShader::operator=(ComputeShader&& other) noexcept {
    if (this != &other) {
        if (programId_ != 0) {
            glDeleteProgram(programId_);
        }
        programId_ = other.programId_;
        other.programId_ = 0;
    }
    return *this;
}

void ComputeShader::Bind() const {
    glUseProgram(programId_);
}

void ComputeShader::Unbind() const {
    glUseProgram(0);
}

void ComputeShader::Dispatch(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ) const {
    glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
}

void ComputeShader::DispatchAndWait(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ) const {
    glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
}

void ComputeShader::SetInt(const std::string& name, int value) const {
    glUniform1i(GetUniformLocation(name), value);
}

void ComputeShader::SetFloat(const std::string& name, float value) const {
    glUniform1f(GetUniformLocation(name), value);
}

void ComputeShader::SetVec2(const std::string& name, const glm::vec2& value) const {
    glUniform2fv(GetUniformLocation(name), 1, glm::value_ptr(value));
}

void ComputeShader::SetVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(GetUniformLocation(name), 1, glm::value_ptr(value));
}

void ComputeShader::SetVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(GetUniformLocation(name), 1, glm::value_ptr(value));
}

void ComputeShader::SetMat4(const std::string& name, const glm::mat4& value) const {
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void ComputeShader::BindImageTexture(uint32_t unit, uint32_t texture, uint32_t access, uint32_t format) const {
    glBindImageTexture(unit, texture, 0, GL_FALSE, 0, access, format);
}

int ComputeShader::GetUniformLocation(const std::string& name) const {
    return glGetUniformLocation(programId_, name.c_str());
}

std::shared_ptr<ComputeShader> ComputeShader::CreateFromFile(const std::string& path) {
    SE_LOG_INFO("Loading compute shader from: {}", path);
    
    std::ifstream file(path);
    if (!file.is_open()) {
        SE_LOG_ERROR("Failed to open compute shader file: {}", path);
        return nullptr;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    return std::make_shared<ComputeShader>(buffer.str());
}

bool ComputeShader::LoadFromSource(const std::string& source) {
    if (programId_ != 0) {
        glDeleteProgram(programId_);
        programId_ = 0;
    }
    
    // Compile compute shader
    GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
    const char* src = source.c_str();
    glShaderSource(computeShader, 1, &src, nullptr);
    glCompileShader(computeShader);
    
    // Check compilation
    GLint success;
    glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(computeShader, 1024, nullptr, infoLog);
        SE_LOG_ERROR("Compute shader compilation failed: {}", infoLog);
        glDeleteShader(computeShader);
        return false;
    }
    
    // Link program
    programId_ = glCreateProgram();
    glAttachShader(programId_, computeShader);
    glLinkProgram(programId_);
    
    glGetProgramiv(programId_, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(programId_, 1024, nullptr, infoLog);
        SE_LOG_ERROR("Compute shader linking failed: {}", infoLog);
        glDeleteProgram(programId_);
        programId_ = 0;
        glDeleteShader(computeShader);
        return false;
    }
    
    glDeleteShader(computeShader);
    SE_LOG_INFO("Compute shader loaded from source (ID: {})", programId_);
    return true;
}

}  // namespace se
