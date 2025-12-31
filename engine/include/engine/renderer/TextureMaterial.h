#pragma once

#include <glm.hpp>
#include <memory>

namespace se {

class Texture;

// DEPRECATED: Use MaterialInstance with MaterialDefinition instead
// This struct is kept for backward compatibility during migration
// See: engine/include/engine/renderer/MaterialInstance.h
struct [[deprecated("Use MaterialInstance instead")]] TextureMaterial {
    std::shared_ptr<Texture> Albedo;
    std::shared_ptr<Texture> Normal;
    std::shared_ptr<Texture> Specular;
    std::shared_ptr<Texture> AO;
    std::shared_ptr<Texture> Emissive;
    std::shared_ptr<Texture> Roughness;
    std::shared_ptr<Texture> Metallic;

    glm::vec4 BaseColor{1.0f};
    glm::vec3 EmissiveColor{0.0f};
    float MetallicFactor = 0.0f;
    float RoughnessFactor = 0.5f;
    float Shininess = 32.0f;

    bool HasAlbedo() const { return Albedo != nullptr; }
    bool HasNormal() const { return Normal != nullptr; }
    bool HasSpecular() const { return Specular != nullptr; }
    bool HasAO() const { return AO != nullptr; }
    bool HasEmissive() const { return Emissive != nullptr; }
    bool HasRoughness() const { return Roughness != nullptr; }
    bool HasMetallic() const { return Metallic != nullptr; }
    
    bool HasAnyTexture() const {
        return HasAlbedo() || HasNormal() || HasSpecular() || HasAO() ||
               HasEmissive() || HasRoughness() || HasMetallic();
    }
};

}  // namespace se
