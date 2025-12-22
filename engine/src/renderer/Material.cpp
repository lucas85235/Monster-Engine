#include <cstring>
#include <string>

#include "engine/core/Application.h"
#include "engine/core/Log.h"
#include "engine/renderer/GraphicsContext.h"
namespace se {

static RHI::IDevice* GetDevice() {
    auto& app     = Application::Get();
    auto* context = app.GetWindow().GetContext();
    return context ? context->GetDevice() : nullptr;
}

Material::Material(const std::shared_ptr<Shader>& shader) : shader_(shader) {
    if (!shader_) return;

    // Initialize Uniform Buffers based on Reflection Data
    const auto& reflection = shader_->GetReflectionData();
    for (const auto& ubo : reflection.uniformBuffers) {
        uniformBuffers_[ubo.binding].resize(ubo.size);
        // Zero initialize
        memset(uniformBuffers_[ubo.binding].data(), 0, ubo.size);
    }
}

void Material::Bind() const {
    if (!shader_) return;

    // Upload Uniform Buffers to GPU (via RHI abstraction in future, currently direct)
    // For now, since RHI doesn't support updating UBOs from Material directly without a Pipeline/DescriptorSet,
    // we will maintain the legacy behavior of setting uniforms individually for OpenGL compatibility layer.
    // BUT! We will read from our local buffer.

    // Legacy Fallback: Iterate over reflection data and set uniforms one by one
    // This bridges the gap until full Vulkan descriptor sets are used.
    auto* device = GetDevice();
    if (!device) return;

    const auto& reflection = shader_->GetReflectionData();
    for (const auto& ubo : reflection.uniformBuffers) {
        const auto& buffer = uniformBuffers_.at(ubo.binding);

        for (const auto& member : ubo.members) {
            // Unpack data from buffer
            if (member.type == ReflectionDataType::Float) {
                float val;
                memcpy(&val, buffer.data() + member.offset, sizeof(float));
                shader_->setFloat(member.name.c_str(), val);
            } else if (member.type == ReflectionDataType::Int) {
                int val;
                memcpy(&val, buffer.data() + member.offset, sizeof(int));
                shader_->setInt(member.name.c_str(), val);
            } else if (member.type == ReflectionDataType::Float3) {
                Vector3 val;
                memcpy(&val, buffer.data() + member.offset, sizeof(Vector3));
                shader_->setVec3(member.name.c_str(), val);
            } else if (member.type == ReflectionDataType::Float4) {
                Vector4 val;
                memcpy(&val, buffer.data() + member.offset, sizeof(Vector4));
                shader_->setVec4(member.name.c_str(), val);
            } else if (member.type == ReflectionDataType::Mat4) {
                Matrix4 val;
                memcpy(&val, buffer.data() + member.offset, sizeof(Matrix4));
                shader_->setMat4(member.name.c_str(), val);
            }
        }
    }

    // Bind Textures
    int slot = 0;
    for (const auto& [name, handle] : textures_) {
        if (RHI::IsValid(handle)) {
            // Activate and bind texture
            device->BindTexture(slot, handle);
            // Set sampler uniform to slot index
            shader_->setInt(name.c_str(), slot);
            slot++;
        }
    }
}

RHI::PipelineHandle Material::GetPipeline(const BufferLayout& layout) {
    if (RHI::IsValid(pipeline_)) return pipeline_;

    auto* device = GetDevice();
    if (!device || !shader_) return {0};

    RHI::VertexLayout vertexLayout;
    vertexLayout.stride    = layout.GetStride();
    uint32_t locationIndex = 0;

    for (const auto& element : layout.GetElements()) {
        RHI::VertexAttribute attr;
        attr.location = locationIndex;
        switch (element.Type) {
            case ShaderDataType::Float:
                attr.type = RHI::VertexAttributeType::Float;
                break;
            case ShaderDataType::Float2:
                attr.type = RHI::VertexAttributeType::Float2;
                break;
            case ShaderDataType::Float3:
                attr.type = RHI::VertexAttributeType::Float3;
                break;
            case ShaderDataType::Float4:
                attr.type = RHI::VertexAttributeType::Float4;
                break;
            case ShaderDataType::Int:
                attr.type = RHI::VertexAttributeType::Int;
                break;
            case ShaderDataType::Int2:
                attr.type = RHI::VertexAttributeType::Int2;
                break;
            case ShaderDataType::Int3:
                attr.type = RHI::VertexAttributeType::Int3;
                break;
            case ShaderDataType::Int4:
                attr.type = RHI::VertexAttributeType::Int4;
                break;
            case ShaderDataType::Mat3:
                attr.type = RHI::VertexAttributeType::Float3;
                locationIndex += 2;
                break;  // Approx
            case ShaderDataType::Mat4:
                attr.type = RHI::VertexAttributeType::Float4;
                locationIndex += 3;
                break;  // Approx
            default:
                attr.type = RHI::VertexAttributeType::Float;
                break;
        }
        attr.offset     = element.Offset;
        attr.normalized = element.Normalized;
        vertexLayout.attributes.push_back(attr);
        locationIndex++;
    }

    RHI::PipelineDescriptor desc{};
    desc.depthStencil.depthTestEnable  = true;
    desc.depthStencil.depthWriteEnable = true;
    desc.depthStencil.depthCompareOp   = RHI::CompareOp::Less;
    desc.rasterizer.cullMode           = RHI::CullMode::Back;
    desc.rasterizer.frontFace          = RHI::FrontFace::CounterClockwise;  // GL default is CCW?
    desc.topology                      = RHI::PrimitiveTopology::TriangleList;
    desc.blend.blendEnable             = false;

    RHI::ShaderHandle shaderHandle = shader_->GetHandle();
    if (RHI::IsValid(shaderHandle)) { pipeline_ = device->CreatePipeline(desc, shaderHandle, vertexLayout); }

    return pipeline_;
}

void Material::Unbind() const {
    shader_->unbind();
}

// Helper to find uniform Member
// Returns pair {binding, offset} or {-1, -1} if not found
std::pair<int, int> FindUniformMember(const ShaderReflectionData& reflection, const std::string& name) {
    for (const auto& ubo : reflection.uniformBuffers) {
        for (const auto& member : ubo.members) {
            if (member.name == name) { return {(int)ubo.binding, (int)member.offset}; }
        }
    }

    return {-1, -1};
}

void Material::SetFloat(const std::string& name, float value) {
    if (!shader_) return;
    auto [binding, offset] = FindUniformMember(shader_->GetReflectionData(), name);
    if (binding != -1) {
        memcpy(uniformBuffers_[binding].data() + offset, &value, sizeof(float));
    } else {
        SE_LOG_WARN("Uniform '{}' not found in shader reflection data", name);
    }
}

void Material::SetInt(const std::string& name, int value) {
    if (!shader_) return;
    auto [binding, offset] = FindUniformMember(shader_->GetReflectionData(), name);
    if (binding != -1) {
        memcpy(uniformBuffers_[binding].data() + offset, &value, sizeof(int));
    } else {
        // Attempt to set it specifically if it's a sampler binding or something else, but for now warning
        // Or suppress warning if it's common
        // SE_LOG_WARN("Uniform '{}' not found", name);
    }
}

void Material::SetVector3(const std::string& name, const Vector3& value) {
    if (!shader_) return;
    auto [binding, offset] = FindUniformMember(shader_->GetReflectionData(), name);
    if (binding != -1) { memcpy(uniformBuffers_[binding].data() + offset, &value, sizeof(Vector3)); }
}

void Material::SetVector4(const std::string& name, const Vector4& value) {
    if (!shader_) return;
    auto [binding, offset] = FindUniformMember(shader_->GetReflectionData(), name);
    if (binding != -1) { memcpy(uniformBuffers_[binding].data() + offset, &value, sizeof(Vector4)); }
}

void Material::SetMatrix4(const std::string& name, const Matrix4& value) {
    if (!shader_) return;
    auto [binding, offset] = FindUniformMember(shader_->GetReflectionData(), name);
    if (binding != -1) { 
        memcpy(uniformBuffers_[binding].data() + offset, &value, sizeof(Matrix4)); 
    }
    // Also call shader directly for immediate update (Vulkan push constants)
    shader_->setMat4(name.c_str(), value);
}

void Material::SetTexture(const std::string& name, RHI::TextureHandle texture) {
    if (!shader_) return;
    // We store the texture handle mapped to the uniform name
    textures_[name] = texture;
}

}  // namespace se
