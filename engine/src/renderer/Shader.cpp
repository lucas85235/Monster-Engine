#include "engine/renderer/Shader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "engine/core/Application.h"
#include "engine/core/Log.h"
#include "engine/renderer/GraphicsContext.h"
#include "engine/rhi/shader_cross_compiler.h"

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
    // Load SPIR-V helper
    auto loadSPIRV = [](const std::filesystem::path& path) -> std::vector<uint32_t> {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return {};

        size_t                fileSize = (size_t)file.tellg();
        std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
        file.seekg(0);
        file.read((char*)buffer.data(), fileSize);
        return buffer;
    };

    // Construct SPIR-V paths
    // e.g. assets/shaders/basic.vert -> assets/shaders/spirv/basic.vert.spv
    auto getSpirvPath = [](const std::filesystem::path& sourcePath) -> std::filesystem::path {
        auto filename = sourcePath.filename();
        auto parent   = sourcePath.parent_path();
        return parent / "spirv" / (filename.string() + ".spv");
    };

    auto vertSpirvPath = getSpirvPath(vertPath);
    auto fragSpirvPath = getSpirvPath(fragPath);

    SE_LOG_INFO("Attempting to load SPIR-V from: {}", vertSpirvPath.string());
    SE_LOG_INFO("Attempting to load SPIR-V from: {}", fragSpirvPath.string());

    std::vector<uint32_t> vertBinary = loadSPIRV(vertSpirvPath);
    std::vector<uint32_t> fragBinary = loadSPIRV(fragSpirvPath);

    if (vertBinary.empty()) SE_LOG_WARN("Failed to load vertex SPIR-V: {}", vertSpirvPath.string());
    if (fragBinary.empty()) SE_LOG_WARN("Failed to load fragment SPIR-V: {}", fragSpirvPath.string());

    if (!vertBinary.empty() && !fragBinary.empty()) {
        SE_LOG_INFO("Loading SPIR-V shaders: {} / {}", vertSpirvPath.string(), fragSpirvPath.string());

        // Get current API from device
        auto* device = GetDevice();
        if (!device) {
            SE_LOG_ERROR("Failed to get RHI device");
            return nullptr;
        }
        
        RHI::API currentAPI = device->GetAPI();
        
        if (currentAPI == RHI::API::Vulkan) {
            // For Vulkan: pass SPIR-V binary directly
            SE_LOG_INFO("Using SPIR-V binary for Vulkan shader");
            
            std::vector<RHI::ShaderDescriptor> stages;
            
            RHI::ShaderDescriptor vertDesc;
            vertDesc.stage = RHI::ShaderStage::Vertex;
            vertDesc.spirvBinary = vertBinary;
            vertDesc.useSPIRV = true;
            stages.push_back(vertDesc);
            
            RHI::ShaderDescriptor fragDesc;
            fragDesc.stage = RHI::ShaderStage::Fragment;
            fragDesc.spirvBinary = fragBinary;
            fragDesc.useSPIRV = true;
            stages.push_back(fragDesc);
            
            auto shader = std::make_shared<Shader>();
            shader->handle_ = device->CreateShader(stages);
            
            if (!RHI::IsValid(shader->handle_)) {
                SE_LOG_ERROR("Failed to create Vulkan shader from SPIR-V");
                return nullptr;
            }
            
            // Extract Reflection Data
            auto vertReflection = RHI::ShaderCrossCompiler::Reflect(vertBinary);
            auto fragReflection = RHI::ShaderCrossCompiler::Reflect(fragBinary);
            
            shader->reflectionData_.uniformBuffers = vertReflection.uniformBuffers;
            for (const auto& ubo : fragReflection.uniformBuffers) {
                bool found = false;
                for (const auto& existing : shader->reflectionData_.uniformBuffers) {
                    if (existing.set == ubo.set && existing.binding == ubo.binding) {
                        found = true;
                        break;
                    }
                }
                if (!found) shader->reflectionData_.uniformBuffers.push_back(ubo);
            }
            
            shader->reflectionData_.resources = vertReflection.resources;
            for (const auto& res : fragReflection.resources) { shader->reflectionData_.resources.push_back(res); }
            
            return shader;
        }
        
        // For OpenGL: transpile SPIR-V to GLSL
        RHI::RenderAPI api = RHI::RenderAPI::OpenGL;
        auto vertResult = RHI::ShaderCrossCompiler::Process(vertBinary, api, RHI::ShaderStageType::Vertex, 450);
        auto fragResult = RHI::ShaderCrossCompiler::Process(fragBinary, api, RHI::ShaderStageType::Fragment, 450);

        if (vertResult.success && fragResult.success) {
            auto shader = std::make_shared<Shader>(vertResult.glslSource, fragResult.glslSource);

            // Extract Reflection Data
            auto vertReflection = RHI::ShaderCrossCompiler::Reflect(vertBinary);
            auto fragReflection = RHI::ShaderCrossCompiler::Reflect(fragBinary);

            // Merge into shader
            shader->reflectionData_.uniformBuffers = vertReflection.uniformBuffers;
            // Append fragment UBOs that are unique
            for (const auto& ubo : fragReflection.uniformBuffers) {
                bool found = false;
                for (const auto& existing : shader->reflectionData_.uniformBuffers) {
                    if (existing.set == ubo.set && existing.binding == ubo.binding) {
                        found = true;
                        break;
                    }
                }
                if (!found) shader->reflectionData_.uniformBuffers.push_back(ubo);
            }

            shader->reflectionData_.resources = vertReflection.resources;
            for (const auto& res : fragReflection.resources) { shader->reflectionData_.resources.push_back(res); }

            return shader;
        } else {
            SE_LOG_ERROR("Cross-compilation failed: V:{} F:{}", vertResult.errorMessage, fragResult.errorMessage);
        }
    }

    // Fallback to text loading (Legacy)
    SE_LOG_WARN("SPIR-V not found or failed, falling back to legacy GLSL source loading: {} / {}", vertPath.string(), fragPath.string());

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
