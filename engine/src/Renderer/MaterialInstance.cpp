#include "engine/renderer/MaterialInstance.h"

#include <glad/glad.h>

#include "engine/Log.h"
#include "engine/Shader.h"
#include "engine/renderer/Texture.h"

namespace se {

MaterialInstance::MaterialInstance() = default;

MaterialInstance::~MaterialInstance() {
    ReleaseResources();
}

MaterialInstance::MaterialInstance(MaterialInstance&& other) noexcept
    : definition_(std::move(other.definition_)),
      uboId_(other.uboId_),
      textures_(std::move(other.textures_)),
      dirty_(other.dirty_) {
    other.uboId_ = 0;
}

MaterialInstance& MaterialInstance::operator=(MaterialInstance&& other) noexcept {
    if (this != &other) {
        ReleaseResources();
        definition_ = std::move(other.definition_);
        uboId_ = other.uboId_;
        textures_ = std::move(other.textures_);
        dirty_ = other.dirty_;
        other.uboId_ = 0;
    }
    return *this;
}

std::shared_ptr<MaterialInstance> MaterialInstance::Create(const MaterialDefinition& definition) {
    auto instance = std::make_shared<MaterialInstance>();
    instance->SetDefinition(definition);
    instance->CreateUBO();
    instance->LoadTexturesFromDefinition();
    instance->UpdateUBO();
    SE_LOG_INFO("MaterialInstance created: '{}'", definition.name);
    return instance;
}

std::shared_ptr<MaterialInstance> MaterialInstance::CreateFromTextures(
    const std::string& name,
    std::shared_ptr<Texture> albedo,
    std::shared_ptr<Texture> normal,
    std::shared_ptr<Texture> metallic,
    std::shared_ptr<Texture> roughness,
    std::shared_ptr<Texture> ao,
    std::shared_ptr<Texture> emissive
) {
    auto instance = std::make_shared<MaterialInstance>();
    
    MaterialDefinition def;
    def.name = name;
    def.features = MATERIAL_FEATURE_NONE;
    
    if (albedo) {
        instance->textures_[static_cast<size_t>(TextureSlot::Albedo)] = albedo;
        def.features |= MATERIAL_FEATURE_ALBEDO_MAP;
    }
    if (normal) {
        instance->textures_[static_cast<size_t>(TextureSlot::Normal)] = normal;
        def.features |= MATERIAL_FEATURE_NORMAL_MAP;
    }
    if (metallic) {
        instance->textures_[static_cast<size_t>(TextureSlot::Metallic)] = metallic;
        def.features |= MATERIAL_FEATURE_METALLIC_MAP;
    }
    if (roughness) {
        instance->textures_[static_cast<size_t>(TextureSlot::Roughness)] = roughness;
        def.features |= MATERIAL_FEATURE_ROUGHNESS_MAP;
    }
    if (ao) {
        instance->textures_[static_cast<size_t>(TextureSlot::AO)] = ao;
        def.features |= MATERIAL_FEATURE_AO_MAP;
    }
    if (emissive) {
        instance->textures_[static_cast<size_t>(TextureSlot::Emissive)] = emissive;
        def.features |= MATERIAL_FEATURE_EMISSIVE_MAP;
    }
    
    instance->definition_ = def;
    instance->CreateUBO();
    instance->UpdateUBO();
    
    SE_LOG_INFO("MaterialInstance created from textures: '{}' (features: 0x{:X})", name, def.features);
    return instance;
}

void MaterialInstance::SetDefinition(const MaterialDefinition& def) {
    definition_ = def;
    dirty_ = true;
}

void MaterialInstance::CreateUBO() {
    if (uboId_ != 0) return;
    
    glGenBuffers(1, &uboId_);
    glBindBuffer(GL_UNIFORM_BUFFER, uboId_);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(MaterialGPUData), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void MaterialInstance::UpdateUBO() {
    if (!dirty_ || uboId_ == 0) return;
    
    MaterialGPUData gpuData = definition_.ToGPUData();
    
    for (size_t i = 0; i < textures_.size(); ++i) {
        if (textures_[i]) {
            switch (static_cast<TextureSlot>(i)) {
                case TextureSlot::Albedo: gpuData.hasAlbedoMap = 1; break;
                case TextureSlot::Normal: gpuData.hasNormalMap = 1; break;
                case TextureSlot::Metallic: gpuData.hasMetallicMap = 1; break;
                case TextureSlot::Roughness: gpuData.hasRoughnessMap = 1; break;
                case TextureSlot::AO: gpuData.hasAOMap = 1; break;
                case TextureSlot::Emissive: gpuData.hasEmissiveMap = 1; break;
                default: break;
            }
        }
    }
    
    glBindBuffer(GL_UNIFORM_BUFFER, uboId_);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(MaterialGPUData), &gpuData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    
    dirty_ = false;
}

void MaterialInstance::LoadTexturesFromDefinition() {
    // Texture loading will be handled by MaterialLibrary which has access to TextureManager
    // This method is a placeholder for when we integrate with the texture loading system
}

void MaterialInstance::ReleaseResources() {
    if (uboId_ != 0) {
        glDeleteBuffers(1, &uboId_);
        uboId_ = 0;
    }
    for (auto& tex : textures_) {
        tex.reset();
    }
}

void MaterialInstance::Bind(Shader* shader) const {
    if (!shader) return;
    
    if (dirty_) {
        const_cast<MaterialInstance*>(this)->UpdateUBO();
    }
    
    // Bind UBO to binding point 1 for Material uniform block
    // Shaders can use: layout(std140, binding = 1) uniform MaterialBlock { ... };
    if (uboId_ != 0) {
        glBindBufferBase(GL_UNIFORM_BUFFER, MATERIAL_UBO_BINDING, uboId_);
    }
    
    BindTextures();
    BindUniforms(shader);
}

void MaterialInstance::BindTextures() const {
    for (size_t i = 0; i < textures_.size(); ++i) {
        if (textures_[i]) {
            textures_[i]->Bind(static_cast<uint32_t>(i));
        }
    }
}

void MaterialInstance::BindUniforms(Shader* shader) const {
    if (!shader) return;
    
    const auto& def = definition_;
    
    shader->setVec4("uBaseColor", def.baseColor);
    shader->setFloat("uMetallicFactor", def.metallic);
    shader->setFloat("uRoughnessFactor", def.roughness);
    shader->setFloat("uReflectance", def.reflectance);
    shader->setFloat("uAOFactor", def.ao);
    shader->setVec3("uEmissiveColor", def.emissiveColor);
    shader->setFloat("uEmissiveFactor", def.emissiveFactor);
    shader->setFloat("uNormalScale", def.normalScale);
    
    shader->setFloat("uClearCoat", def.clearCoat);
    shader->setFloat("uClearCoatRoughness", def.clearCoatRoughness);
    shader->setFloat("uAnisotropy", def.anisotropy);
    shader->setFloat("uSubsurfacePower", def.subsurfacePower);
    shader->setFloat("uTransmission", def.transmission);
    shader->setFloat("uIOR", def.ior);
    
    shader->setInt("uAlbedoMap", static_cast<int>(TextureSlot::Albedo));
    shader->setInt("uNormalMap", static_cast<int>(TextureSlot::Normal));
    shader->setInt("uMetallicMap", static_cast<int>(TextureSlot::Metallic));
    shader->setInt("uRoughnessMap", static_cast<int>(TextureSlot::Roughness));
    shader->setInt("uAOMap", static_cast<int>(TextureSlot::AO));
    shader->setInt("uEmissiveMap", static_cast<int>(TextureSlot::Emissive));
    shader->setInt("uSpecularMap", static_cast<int>(TextureSlot::Specular));
    
    int hasAlbedo = textures_[static_cast<size_t>(TextureSlot::Albedo)] ? 1 : 0;
    int hasNormal = textures_[static_cast<size_t>(TextureSlot::Normal)] ? 1 : 0;
    int hasMetallic = textures_[static_cast<size_t>(TextureSlot::Metallic)] ? 1 : 0;
    int hasRoughness = textures_[static_cast<size_t>(TextureSlot::Roughness)] ? 1 : 0;
    int hasAO = textures_[static_cast<size_t>(TextureSlot::AO)] ? 1 : 0;
    int hasEmissive = textures_[static_cast<size_t>(TextureSlot::Emissive)] ? 1 : 0;
    int hasSpecular = textures_[static_cast<size_t>(TextureSlot::Specular)] ? 1 : 0;
    
    shader->setInt("uHasAlbedo", hasAlbedo);
    shader->setInt("uHasNormal", hasNormal);
    shader->setInt("uHasMetallic", hasMetallic);
    shader->setInt("uHasRoughness", hasRoughness);
    shader->setInt("uHasAO", hasAO);
    shader->setInt("uHasEmissive", hasEmissive);
    shader->setInt("uHasSpecular", hasSpecular);
}

Texture* MaterialInstance::GetTexture(TextureSlot slot) const {
    size_t idx = static_cast<size_t>(slot);
    if (idx < textures_.size()) {
        return textures_[idx].get();
    }
    return nullptr;
}

std::shared_ptr<Texture> MaterialInstance::GetTextureShared(TextureSlot slot) const {
    size_t idx = static_cast<size_t>(slot);
    if (idx < textures_.size()) {
        return textures_[idx];
    }
    return nullptr;
}

void MaterialInstance::SetTexture(TextureSlot slot, std::shared_ptr<Texture> texture) {
    size_t idx = static_cast<size_t>(slot);
    if (idx < textures_.size()) {
        textures_[idx] = std::move(texture);
        
        uint32_t feature = MATERIAL_FEATURE_NONE;
        switch (slot) {
            case TextureSlot::Albedo: feature = MATERIAL_FEATURE_ALBEDO_MAP; break;
            case TextureSlot::Normal: feature = MATERIAL_FEATURE_NORMAL_MAP; break;
            case TextureSlot::Metallic: feature = MATERIAL_FEATURE_METALLIC_MAP; break;
            case TextureSlot::Roughness: feature = MATERIAL_FEATURE_ROUGHNESS_MAP; break;
            case TextureSlot::AO: feature = MATERIAL_FEATURE_AO_MAP; break;
            case TextureSlot::Emissive: feature = MATERIAL_FEATURE_EMISSIVE_MAP; break;
            default: break;
        }
        
        if (textures_[idx]) {
            definition_.features |= feature;
        } else {
            definition_.features &= ~feature;
        }
        
        dirty_ = true;
    }
}

bool MaterialInstance::HasTexture(TextureSlot slot) const {
    size_t idx = static_cast<size_t>(slot);
    return idx < textures_.size() && textures_[idx] != nullptr;
}

void MaterialInstance::SetBaseColor(const Vector4& color) {
    definition_.baseColor = color;
    dirty_ = true;
}

void MaterialInstance::SetMetallic(float metallic) {
    definition_.metallic = metallic;
    dirty_ = true;
}

void MaterialInstance::SetRoughness(float roughness) {
    definition_.roughness = roughness;
    dirty_ = true;
}

void MaterialInstance::SetEmissive(const Vector3& color, float factor) {
    definition_.emissiveColor = color;
    definition_.emissiveFactor = factor;
    if (factor > 0.0f) {
        definition_.features |= MATERIAL_FEATURE_EMISSIVE;
    } else {
        definition_.features &= ~MATERIAL_FEATURE_EMISSIVE;
    }
    dirty_ = true;
}

void MaterialInstance::Reload() {
    LoadTexturesFromDefinition();
    dirty_ = true;
    UpdateUBO();
    SE_LOG_INFO("MaterialInstance reloaded: '{}'", definition_.name);
}

}  // namespace se
