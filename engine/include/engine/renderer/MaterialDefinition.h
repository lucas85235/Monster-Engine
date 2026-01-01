#pragma once

#include <cstdint>
#include <string>

#include <glm.hpp>

namespace se {

using Vector3 = glm::vec3;
using Vector4 = glm::vec4;

enum MaterialFeature : uint32_t {
    MATERIAL_FEATURE_NONE           = 0,
    MATERIAL_FEATURE_ALBEDO_MAP     = 1 << 0,
    MATERIAL_FEATURE_NORMAL_MAP     = 1 << 1,
    MATERIAL_FEATURE_METALLIC_MAP   = 1 << 2,
    MATERIAL_FEATURE_ROUGHNESS_MAP  = 1 << 3,
    MATERIAL_FEATURE_AO_MAP         = 1 << 4,
    MATERIAL_FEATURE_EMISSIVE_MAP   = 1 << 5,
    MATERIAL_FEATURE_EMISSIVE       = 1 << 6,
    MATERIAL_FEATURE_CLEAR_COAT     = 1 << 7,
    MATERIAL_FEATURE_ANISOTROPY     = 1 << 8,
    MATERIAL_FEATURE_SUBSURFACE     = 1 << 9,
    MATERIAL_FEATURE_TRANSMISSION   = 1 << 10,
    MATERIAL_FEATURE_ALPHA_BLEND    = 1 << 11,
};

enum class TextureSlot : uint32_t {
    Shadow      = 0,
    Albedo      = 1,
    Normal      = 2,
    Metallic    = 3,
    Roughness   = 4,
    AO          = 5,
    Emissive    = 6,
    Specular    = 7,
    
    IrradianceCubemap   = 8,
    PrefilteredCubemap  = 9,
    DfgLut              = 10,
    
    GI          = 11,
    
    Count       = 12
};

#pragma pack(push, 1)
struct alignas(16) MaterialGPUData {
    alignas(16) Vector4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
    alignas(16) Vector4 emissive{0.0f, 0.0f, 0.0f, 0.0f};
    
    float metallic = 0.0f;
    float roughness = 0.5f;
    float reflectance = 0.5f;
    float ao = 1.0f;
    
    float normalScale = 1.0f;
    float clearCoat = 0.0f;
    float clearCoatRoughness = 0.0f;
    float anisotropy = 0.0f;
    
    float subsurfacePower = 0.0f;
    float transmission = 0.0f;
    float ior = 1.5f;
    float _padding1 = 0.0f;
    
    int32_t hasAlbedoMap = 0;
    int32_t hasNormalMap = 0;
    int32_t hasMetallicMap = 0;
    int32_t hasRoughnessMap = 0;
    
    int32_t hasAOMap = 0;
    int32_t hasEmissiveMap = 0;
    int32_t _padding2 = 0;
    int32_t _padding3 = 0;
};
#pragma pack(pop)

static_assert(sizeof(MaterialGPUData) % 16 == 0, "MaterialGPUData must be 16-byte aligned for UBO");

struct MaterialDefinition {
    std::string name = "Default Material";
    std::string uuid;
    std::string sourcePath;
    
    Vector4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
    Vector3 emissiveColor{0.0f, 0.0f, 0.0f};
    float emissiveFactor = 0.0f;
    
    float metallic = 0.0f;
    float roughness = 0.5f;
    float reflectance = 0.5f;
    float ao = 1.0f;
    float normalScale = 1.0f;
    
    float clearCoat = 0.0f;
    float clearCoatRoughness = 0.0f;
    
    float anisotropy = 0.0f;
    Vector3 anisotropyDirection{1.0f, 0.0f, 0.0f};
    
    Vector3 sheenColor{0.0f, 0.0f, 0.0f};
    float sheenRoughness = 0.0f;
    
    Vector3 subsurfaceColor{0.0f, 0.0f, 0.0f};
    float subsurfacePower = 0.0f;
    float thickness = 0.0f;
    
    float transmission = 0.0f;
    float ior = 1.5f;
    
    std::string albedoPath;
    std::string normalPath;
    std::string metallicPath;
    std::string roughnessPath;
    std::string aoPath;
    std::string emissivePath;
    
    uint32_t features = MATERIAL_FEATURE_NONE;
    
    MaterialGPUData ToGPUData() const {
        MaterialGPUData data;
        data.baseColor = baseColor;
        data.emissive = Vector4(emissiveColor, emissiveFactor);
        data.metallic = metallic;
        data.roughness = roughness;
        data.reflectance = reflectance;
        data.ao = ao;
        data.normalScale = normalScale;
        data.clearCoat = clearCoat;
        data.clearCoatRoughness = clearCoatRoughness;
        data.anisotropy = anisotropy;
        data.subsurfacePower = subsurfacePower;
        data.transmission = transmission;
        data.ior = ior;
        
        data.hasAlbedoMap = (features & MATERIAL_FEATURE_ALBEDO_MAP) ? 1 : 0;
        data.hasNormalMap = (features & MATERIAL_FEATURE_NORMAL_MAP) ? 1 : 0;
        data.hasMetallicMap = (features & MATERIAL_FEATURE_METALLIC_MAP) ? 1 : 0;
        data.hasRoughnessMap = (features & MATERIAL_FEATURE_ROUGHNESS_MAP) ? 1 : 0;
        data.hasAOMap = (features & MATERIAL_FEATURE_AO_MAP) ? 1 : 0;
        data.hasEmissiveMap = (features & MATERIAL_FEATURE_EMISSIVE_MAP) ? 1 : 0;
        
        return data;
    }
    
    void UpdateFeaturesFromPaths() {
        features = MATERIAL_FEATURE_NONE;
        if (!albedoPath.empty()) features |= MATERIAL_FEATURE_ALBEDO_MAP;
        if (!normalPath.empty()) features |= MATERIAL_FEATURE_NORMAL_MAP;
        if (!metallicPath.empty()) features |= MATERIAL_FEATURE_METALLIC_MAP;
        if (!roughnessPath.empty()) features |= MATERIAL_FEATURE_ROUGHNESS_MAP;
        if (!aoPath.empty()) features |= MATERIAL_FEATURE_AO_MAP;
        if (!emissivePath.empty()) features |= MATERIAL_FEATURE_EMISSIVE_MAP;
        if (emissiveFactor > 0.0f) features |= MATERIAL_FEATURE_EMISSIVE;
        if (clearCoat > 0.0f) features |= MATERIAL_FEATURE_CLEAR_COAT;
        if (std::abs(anisotropy) > 0.01f) features |= MATERIAL_FEATURE_ANISOTROPY;
        if (subsurfacePower > 0.0f) features |= MATERIAL_FEATURE_SUBSURFACE;
        if (transmission > 0.0f) features |= MATERIAL_FEATURE_TRANSMISSION;
        if (baseColor.a < 1.0f) features |= MATERIAL_FEATURE_ALPHA_BLEND;
    }
    
    static MaterialDefinition CreateDefault() {
        MaterialDefinition def;
        def.name = "Default";
        return def;
    }
    
    // Editor integration helpers
    void CopyFromEditorData(const void* editorDataPtr) {
        // Note: This is a type-erased interface for editor integration
        // The actual EditorMaterialData struct fields match our fields 1:1
    }
    
    void CopyToEditorData(void* editorDataPtr) const {
        // Note: This is a type-erased interface for editor integration
    }
};

}  // namespace se
