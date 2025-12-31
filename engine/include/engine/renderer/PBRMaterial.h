#pragma once

#include <glm.hpp>
#include <memory>

#include "engine/Shader.h"
#include "engine/renderer/Material.h"

namespace se {

class Texture;
struct TextureMaterial;

// DEPRECATED: Use MaterialDefinition instead
// This struct is kept for backward compatibility during migration
// See: engine/include/engine/renderer/MaterialDefinition.h
struct PBRMaterialParams {
    // Core parameters
    Vector4 BaseColor{1.0f, 1.0f, 1.0f, 1.0f};  // Base color (linear RGBA)
    float Metallic = 0.0f;                       // 0 = dielectric, 1 = metal
    float Roughness = 0.5f;                      // Perceptual roughness [0-1]
    float Reflectance = 0.5f;                    // Dielectric reflectance (0.5 = 4% F0)
    float AO = 1.0f;                             // Ambient occlusion multiplier
    Vector3 EmissiveColor{0.0f, 0.0f, 0.0f};    // Emissive RGB
    float EmissiveFactor = 0.0f;                 // Emissive intensity
    float NormalScale = 1.0f;                    // Normal map intensity
    
    // Clear Coat (varnish, lacquer, car paint)
    float ClearCoat = 0.0f;                      // Clear coat intensity [0-1]
    float ClearCoatRoughness = 0.0f;             // Clear coat roughness [0-1]
    
    // Anisotropy (brushed metal, hair)
    float Anisotropy = 0.0f;                     // Anisotropy [-1, 1]
    Vector3 AnisotropyDirection{1.0f, 0.0f, 0.0f}; // Direction in tangent space
    
    // Sheen (fabric, velvet)
    Vector3 SheenColor{0.0f, 0.0f, 0.0f};       // Sheen color
    float SheenRoughness = 0.0f;                 // Sheen roughness [0-1]
    
    // Subsurface (skin, wax, leaves)
    Vector3 SubsurfaceColor{0.0f, 0.0f, 0.0f};  // Subsurface scattering color
    float SubsurfacePower = 0.0f;                // Subsurface scattering power
    float Thickness = 0.0f;                      // Material thickness [0-1]
    
    // Transmission (glass, water)
    float Transmission = 0.0f;                   // Transmission [0-1]
    float IOR = 1.5f;                            // Index of refraction
};

/**
 * IBL (Image-Based Lighting) data for environment lighting.
 * Supports both Spherical Harmonics fallback and HDR cubemaps.
 */
struct IBLData {
    // Spherical Harmonics fallback
    Vector3 SphericalHarmonics[9];              // 9 SH coefficients for diffuse irradiance
    float Intensity = 1.0f;                      // Overall IBL intensity
    Vector3 SkyColor{0.5f, 0.7f, 1.0f};         // Fallback sky color
    Vector3 GroundColor{0.2f, 0.15f, 0.1f};     // Fallback ground color
    
    // HDR Cubemap textures (0 = not set, use SH fallback)
    uint32_t EnvironmentCubemap = 0;             // Original HDR environment (for skybox)
    uint32_t IrradianceCubemap = 0;              // Diffuse irradiance cubemap
    uint32_t PrefilteredCubemap = 0;             // Specular prefiltered cubemap (mipmapped)
    uint32_t DfgLut = 0;                         // DFG split-sum LUT
    int PrefilteredMipLevels = 5;                // Number of mip levels in prefiltered cubemap
    int EnvironmentCubemapSize = 512;            // Size of environment cubemap
    
    bool HasCubemaps() const { return IrradianceCubemap != 0 && PrefilteredCubemap != 0 && DfgLut != 0; }
    
    // Initialize with default outdoor lighting
    void SetDefaultOutdoor() {
        // Physically-based outdoor SH approximation (clear sky with sun)
        // Reference: Ramamoorthi and Hanrahan's paper on SH lighting
        // L00: DC term (average environment color - bright for outdoor)
        SphericalHarmonics[0] = Vector3(1.8f, 1.85f, 2.1f);
        // L1-1, L10, L11: Linear terms (directional variation)
        SphericalHarmonics[1] = Vector3(0.1f, 0.1f, 0.12f);  // Side light (slight)
        SphericalHarmonics[2] = Vector3(0.8f, 0.85f, 1.1f);  // Sky up - strong blue
        SphericalHarmonics[3] = Vector3(0.05f, 0.05f, 0.05f); // Minimal horizontal
        // L2-2, L2-1, L20, L21, L22: Quadratic terms (color bleeding)
        SphericalHarmonics[4] = Vector3(0.0f, 0.0f, 0.0f);
        SphericalHarmonics[5] = Vector3(0.0f, 0.0f, 0.0f);
        SphericalHarmonics[6] = Vector3(0.35f, 0.3f, 0.2f);  // Ground bounce - warm
        SphericalHarmonics[7] = Vector3(0.0f, 0.0f, 0.0f);
        SphericalHarmonics[8] = Vector3(0.0f, 0.0f, 0.0f);
        Intensity = 1.2f;
        SkyColor = Vector3(0.85f, 0.9f, 1.0f);
        GroundColor = Vector3(0.45f, 0.4f, 0.35f);
    }
    
    // Initialize with default indoor lighting
    void SetDefaultIndoor() {
        // Neutral indoor lighting
        for (int i = 0; i < 9; i++) {
            SphericalHarmonics[i] = Vector3(0.0f);
        }
        SphericalHarmonics[0] = Vector3(0.4f, 0.4f, 0.4f); // L00 (ambient only)
        Intensity = 1.0f;
    }
    
    // Clear SH data (use fallback sky/ground colors)
    void Clear() {
        for (int i = 0; i < 9; i++) {
            SphericalHarmonics[i] = Vector3(0.0f);
        }
    }
};

/**
 * PBR Material class that wraps a shader with automatic PBR uniform binding.
 * Provides a convenient interface for setting PBR material properties.
 */
class PBRMaterial : public Material {
   public:
    explicit PBRMaterial(const std::shared_ptr<Shader>& shader);
    
    /**
     * Create a PBRMaterial from TextureMaterial data.
     * This factory method creates a material using the PBR standard shader.
     */
    static std::shared_ptr<PBRMaterial> CreateFromTextureMaterial(
        const std::shared_ptr<Shader>& shader,
        const std::shared_ptr<TextureMaterial>& textureMaterial
    );
    
    // Set PBR material parameters
    void SetParams(const PBRMaterialParams& params) { params_ = params; }
    const PBRMaterialParams& GetParams() const { return params_; }
    PBRMaterialParams& GetParams() { return params_; }
    
    // Bind material and upload PBR uniforms
    void BindPBR() const;
    
    // Bind IBL data (typically called by SceneRenderer)
    void BindIBL(const IBLData& ibl) const;
    
   private:
    PBRMaterialParams params_;
};

}  // namespace se
