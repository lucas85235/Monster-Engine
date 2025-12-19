#include "engine/core/Application.h"
#include "engine/core/Log.h"
#include "engine/renderer/GraphicsContext.h"

namespace se {

static RHI::IDevice* GetDevice() {
    auto& app     = Application::Get();
    auto* context = app.GetWindow().GetContext();
    return context ? context->GetDevice() : nullptr;
}

Material::Material(const std::shared_ptr<Shader>& shader) : shader_(shader) {}

void Material::Bind() const {
    // shader_->bind() is no-op now.
    // Use GetPipeline() and BindPipeline handling in SceneRenderer instead.
    // Or call BindUniforms() if we split it. For now, keep SetUniform calls here which use RHI.

    // Apply uniforms
    for (const auto& [name, value] : intUniforms_) { shader_->setInt(name.c_str(), value); }
    for (const auto& [name, value] : floatUniforms_) { shader_->setFloat(name.c_str(), value); }
    for (const auto& [name, value] : vec3Uniforms_) { shader_->setVec3(name.c_str(), value); }
    for (const auto& [name, value] : vec4Uniforms_) { shader_->setVec4(name.c_str(), value); }
    for (const auto& [name, value] : mat4Uniforms_) { shader_->setMat4(name.c_str(), value); }
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
