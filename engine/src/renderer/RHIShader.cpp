#include "engine/renderer/RHIShader.h"

#include <fstream>
#include <sstream>

#include "engine/core/Application.h"
#include "engine/core/Log.h"
#include "engine/renderer/GraphicsContext.h"

namespace se {

static RHI::IDevice* GetDevice() {
    auto& app     = Application::Get();
    auto* context = app.GetWindow().GetContext();
    return context ? context->GetDevice() : nullptr;
}

static std::string ReadFile(const std::string& filepath) {
    std::ifstream in(filepath, std::ios::in | std::ios::binary);
    if (in) {
        std::string contents;
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
        return contents;
    }
    SE_LOG_ERROR("Could not open shader file: {}", filepath);
    return "";
}

RHIShader::RHIShader(const std::string& vertPath, const std::string& fragPath) {
    std::string vertSrc = ReadFile(vertPath);
    std::string fragSrc = ReadFile(fragPath);

    if (vertSrc.empty() || fragSrc.empty()) { return; }

    std::vector<RHI::ShaderDescriptor> stages;

    RHI::ShaderDescriptor vertDesc{};
    vertDesc.stage  = RHI::ShaderStage::Vertex;
    vertDesc.source = vertSrc;
    // Note: For Vulkan, we would need SPIR-V or online compilation here.
    // Currently passing source assumes OpenGL or that Device handles generic source (which our current RHI adapter for OpenGL does).
    stages.push_back(vertDesc);

    RHI::ShaderDescriptor fragDesc{};
    fragDesc.stage  = RHI::ShaderStage::Fragment;
    fragDesc.source = fragSrc;
    stages.push_back(fragDesc);

    auto* device = GetDevice();
    if (device) { handle_ = device->CreateShader(stages); }
}

RHIShader::~RHIShader() {
    auto* device = GetDevice();
    if (device && RHI::IsValid(handle_)) { device->DestroyShader(handle_); }
}

void RHIShader::Bind() const {
    SE_LOG_WARN("RHIShader::Bind called. RHI does not support binding shaders directly. Use Pipeline.");
}

void RHIShader::SetFloat(const std::string& name, float value) {
    if (auto* dev = GetDevice()) dev->SetUniform(handle_, name, value);
}

void RHIShader::SetInt(const std::string& name, int value) {
    if (auto* dev = GetDevice()) dev->SetUniform(handle_, name, value);
}

void RHIShader::SetMat4(const std::string& name, const glm::mat4& value) {
    if (auto* dev = GetDevice()) dev->SetUniformMatrix4(handle_, name, &value[0][0]);
}

void RHIShader::SetVec3(const std::string& name, const glm::vec3& value) {
    if (auto* dev = GetDevice()) dev->SetUniform(handle_, name, &value[0], 3);
}

void RHIShader::SetVec4(const std::string& name, const glm::vec4& value) {
    if (auto* dev = GetDevice()) dev->SetUniform(handle_, name, &value[0], 4);
}

std::shared_ptr<RHIShader> RHIShader::Create(const std::string& vertPath, const std::string& fragPath) {
    return std::make_shared<RHIShader>(vertPath, fragPath);
}

}  // namespace se
