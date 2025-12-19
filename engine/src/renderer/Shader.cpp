#include "engine/renderer/Shader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "engine/core/Application.h"
#include "engine/core/Log.h"
#include "engine/renderer/GraphicsContext.h"

namespace se {

static RHI::IDevice* GetDevice() {
    auto& app     = Application::Get();
    auto* context = app.GetWindow().GetContext();
    return context ? context->GetDevice() : nullptr;
}

Shader::Shader(const std::string& vertSrc, const std::string& fragSrc) {
    auto* device = GetDevice();
    if (!device) {
        SE_LOG_ERROR("Failed to get RHI device for shader creation");
        return;
    }

    std::vector<RHI::ShaderDescriptor> stages;

    RHI::ShaderDescriptor vertDesc;
    vertDesc.stage  = RHI::ShaderStage::Vertex;
    vertDesc.source = vertSrc;
    stages.push_back(vertDesc);

    RHI::ShaderDescriptor fragDesc;
    fragDesc.stage  = RHI::ShaderStage::Fragment;
    fragDesc.source = fragSrc;
    stages.push_back(fragDesc);

    handle_ = device->CreateShader(stages);

    if (RHI::IsValid(handle_)) {
        // SE_LOG_INFO("Shader created successfully via RHI");
    } else {
        SE_LOG_ERROR("Failed to create shader via RHI");
    }
}

Shader::~Shader() {
    auto* device = GetDevice();
    if (device && RHI::IsValid(handle_)) { device->DestroyShader(handle_); }
}

void Shader::bind() const {
    // No-op. In RHI, binding is done via Pipelines.
    // Legacy code calling this expects GL state change, but we moved to Pipeline model.
    // Warning: If caller relies on this setting program for loose uniform setting or draw calls without pipeline, it will fail.
}

void Shader::unbind() const {
    // No-op.
}

void Shader::setFloat(const char* name, float value) const {
    if (auto* dev = GetDevice()) dev->SetUniform(handle_, name, value);
}

void Shader::setInt(const char* name, int value) const {
    if (auto* dev = GetDevice()) dev->SetUniform(handle_, name, value);
}

void Shader::setVec3(const char* name, const Vector3& value) const {
    if (auto* dev = GetDevice()) dev->SetUniform(handle_, name, &value[0], 3);
}

void Shader::setVec4(const char* name, const Vector4& value) const {
    if (auto* dev = GetDevice()) dev->SetUniform(handle_, name, &value[0], 4);
}

void Shader::setMat4(const char* name, const Matrix4& value) const {
    if (auto* dev = GetDevice()) dev->SetUniformMatrix4(handle_, name, &value[0][0]);
}

std::shared_ptr<Shader> Shader::CreateFromFiles(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath) {
    // Read files helper
    auto loadFile = [](const std::filesystem::path& path) -> std::string {
        std::ifstream file(path);
        if (!file.is_open()) return "";
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    };

    std::string vertSrc = loadFile(vertPath);
    std::string fragSrc = loadFile(fragPath);

    if (vertSrc.empty() || fragSrc.empty()) {
        SE_LOG_ERROR("Failed to load shader files: {} / {}", vertPath.string(), fragPath.string());
        return nullptr;
    }

    return std::make_shared<Shader>(vertSrc, fragSrc);
}

}  // namespace se
