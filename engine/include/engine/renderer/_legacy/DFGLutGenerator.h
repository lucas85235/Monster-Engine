#pragma once
/**
 * DFGLutGenerator.h - Generates pre-computed DFG Look-Up Table for IBL
 * 
 * The DFG LUT stores pre-integrated BRDF terms for the split-sum approximation:
 * - R channel: Scale factor for F0 (fresnel at normal incidence)
 * - G channel: Bias for F90 (fresnel at grazing angle)
 * 
 * This eliminates the need for runtime importance sampling and provides
 * accurate energy compensation for multi-scattering (Kulla-Conty).
 */

#include <memory>
#include <cstdint>

namespace se {

class Texture;

class DFGLutGenerator {
public:
    static constexpr int LUT_SIZE = 512;
    
    static std::shared_ptr<Texture> Generate();
    
    static std::shared_ptr<Texture> LoadOrGenerate(const std::string& cachePath);
    
    static bool SaveToFile(const std::shared_ptr<Texture>& lut, const std::string& path);

private:
    static float RadicalInverse_VdC(uint32_t bits);
    static void Hammersley(uint32_t i, uint32_t N, float& xi1, float& xi2);
    static void ImportanceSampleGGX(float xi1, float xi2, float roughness, float& NoH, float& VoH);
    static float GeometrySchlickGGX(float NdotV, float roughness);
    static float GeometrySmith(float NoV, float NoL, float roughness);
    static void IntegrateBRDF(float NoV, float roughness, float& scale, float& bias);
};

} // namespace se
