#pragma once

#include <array>
#include <memory>
#include <string>

#include "engine/renderer/MaterialDefinition.h"

namespace se {

class Shader;
class Texture;

class MaterialInstance {
public:
    MaterialInstance();
    ~MaterialInstance();
    
    MaterialInstance(const MaterialInstance&) = delete;
    MaterialInstance& operator=(const MaterialInstance&) = delete;
    MaterialInstance(MaterialInstance&& other) noexcept;
    MaterialInstance& operator=(MaterialInstance&& other) noexcept;
    
    static std::shared_ptr<MaterialInstance> Create(const MaterialDefinition& definition);
    
    static std::shared_ptr<MaterialInstance> CreateFromTextures(
        const std::string& name,
        std::shared_ptr<Texture> albedo,
        std::shared_ptr<Texture> normal = nullptr,
        std::shared_ptr<Texture> metallic = nullptr,
        std::shared_ptr<Texture> roughness = nullptr,
        std::shared_ptr<Texture> ao = nullptr,
        std::shared_ptr<Texture> emissive = nullptr
    );
    
    void Bind(Shader* shader) const;
    
    void BindTextures() const;
    void BindUniforms(Shader* shader) const;
    
    const MaterialDefinition& GetDefinition() const { return definition_; }
    MaterialDefinition& GetDefinition() { return definition_; }
    
    void SetDefinition(const MaterialDefinition& def);
    
    Texture* GetTexture(TextureSlot slot) const;
    void SetTexture(TextureSlot slot, std::shared_ptr<Texture> texture);
    
    bool HasTexture(TextureSlot slot) const;
    
    void SetBaseColor(const Vector4& color);
    void SetMetallic(float metallic);
    void SetRoughness(float roughness);
    void SetEmissive(const Vector3& color, float factor);
    
    void MarkDirty() { dirty_ = true; }
    bool IsDirty() const { return dirty_; }
    
    void Reload();
    
    uint32_t GetUBOId() const { return uboId_; }
    
private:
    void CreateUBO();
    void UpdateUBO();
    void LoadTexturesFromDefinition();
    void ReleaseResources();
    
    MaterialDefinition definition_;
    
    uint32_t uboId_ = 0;
    
    std::array<std::shared_ptr<Texture>, static_cast<size_t>(TextureSlot::Count)> textures_;
    
    mutable bool dirty_ = true;
    
    static constexpr uint32_t MATERIAL_UBO_BINDING = 1;
};

}  // namespace se
