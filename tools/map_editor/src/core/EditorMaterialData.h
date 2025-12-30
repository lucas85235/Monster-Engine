#pragma once
/**
 * EditorMaterialData.h - Material data structures for the map editor.
 *
 * Defines the complete PBR material specification that can be:
 * - Edited in the Material Editor UI
 * - Serialized to .mstmat files
 * - Assigned to entities in the scene
 */

#include <cstdint>
#include <string>

#include "Engine.h"

namespace mst {

using se::Vector3;
using se::Vector4;

/**
 * Complete PBR material definition for editor storage.
 * Mirrors the engine's PBRMaterialParams with additional editor metadata.
 */
struct EditorMaterialData {
    std::string name = "New Material";
    std::string uuid;
    
    // Core PBR Parameters
    Vector4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
    float metallic = 0.0f;
    float roughness = 0.5f;
    float reflectance = 0.5f;
    float ao = 1.0f;
    float normalScale = 1.0f;
    
    // Emissive
    Vector3 emissiveColor{0.0f, 0.0f, 0.0f};
    float emissiveFactor = 0.0f;
    
    // Clear Coat
    float clearCoat = 0.0f;
    float clearCoatRoughness = 0.0f;
    
    // Anisotropy
    float anisotropy = 0.0f;
    Vector3 anisotropyDirection{1.0f, 0.0f, 0.0f};
    
    // Sheen (fabric)
    Vector3 sheenColor{0.0f, 0.0f, 0.0f};
    float sheenRoughness = 0.0f;
    
    // Subsurface
    Vector3 subsurfaceColor{0.0f, 0.0f, 0.0f};
    float subsurfacePower = 0.0f;
    float thickness = 0.0f;
    
    // Transmission (glass)
    float transmission = 0.0f;
    float ior = 1.5f;
    
    // Texture paths (relative to project/assets folder)
    std::string albedoTexturePath;
    std::string normalTexturePath;
    std::string metallicTexturePath;
    std::string roughnessTexturePath;
    std::string aoTexturePath;
    std::string emissiveTexturePath;
    
    // Texture usage flags
    bool useAlbedoTexture = false;
    bool useNormalTexture = false;
    bool useMetallicTexture = false;
    bool useRoughnessTexture = false;
    bool useAOTexture = false;
    bool useEmissiveTexture = false;
    
    // Editor metadata
    bool isDirty = false;
    std::string filePath;
    
    bool HasAnyTexture() const {
        return useAlbedoTexture || useNormalTexture || useMetallicTexture ||
               useRoughnessTexture || useAOTexture || useEmissiveTexture;
    }
    
    void Reset() {
        *this = EditorMaterialData{};
    }
};

/**
 * Material reference for entities.
 * Stored in MapEntityData to link entities to materials.
 */
struct MaterialReference {
    std::string materialPath;
    bool hasCustomMaterial = false;
    
    // Per-instance overrides (optional)
    bool overrideBaseColor = false;
    Vector4 baseColorOverride{1.0f};
    
    bool overrideRoughness = false;
    float roughnessOverride = 0.5f;
    
    bool overrideMetallic = false;
    float metallicOverride = 0.0f;
};

}  // namespace mst
