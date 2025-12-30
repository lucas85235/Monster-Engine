#pragma once

#include <glm.hpp>
#include <memory>

#include "engine/Shader.h"
#include "engine/renderer/Material.h"

namespace se {

class Texture;
struct TextureMaterial;

/**
 * PBR Material Parameters following Filament's Standard Model.
 * These are the artist-friendly parameters exposed to the user.
 */
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
        // Outdoor SH approximation - strong ambient to fill shadows realistically
        // L00 is the dominant ambient term - needs to be bright enough to fill shadow areas
        SphericalHarmonics[0] = Vector3(2.0f, 2.0f, 2.2f);   // L00 (dominant ambient - very bright)
        SphericalHarmonics[1] = Vector3(0.0f, 0.0f, 0.0f);   // L1-1
        SphericalHarmonics[2] = Vector3(0.6f, 0.7f, 0.9f);   // L10 (sky up contribution - blue tint)
        SphericalHarmonics[3] = Vector3(0.0f, 0.0f, 0.0f);   // L11
        SphericalHarmonics[4] = Vector3(0.0f, 0.0f, 0.0f);   // L2-2
        SphericalHarmonics[5] = Vector3(0.0f, 0.0f, 0.0f);   // L2-1
        SphericalHarmonics[6] = Vector3(0.2f, 0.15f, 0.1f);  // L20 (ground bounce - warm brown)
        SphericalHarmonics[7] = Vector3(0.0f, 0.0f, 0.0f);   // L21
        SphericalHarmonics[8] = Vector3(0.0f, 0.0f, 0.0f);   // L22
        Intensity = 1.5f;  // Increased intensity for shadow fill
        SkyColor = Vector3(0.9f, 0.95f, 1.0f);   // Bright sky fallback
        GroundColor = Vector3(0.4f, 0.35f, 0.3f); // Warm ground fallback
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
