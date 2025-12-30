#include "engine/renderer/PBRMaterial.h"

#include "engine/Log.h"
#include "engine/renderer/TextureMaterial.h"
#include "engine/renderer/Texture.h"

namespace se {

PBRMaterial::PBRMaterial(const std::shared_ptr<Shader>& shader)
    : Material(shader) {
}

std::shared_ptr<PBRMaterial> PBRMaterial::CreateFromTextureMaterial(
    const std::shared_ptr<Shader>& shader,
    const std::shared_ptr<TextureMaterial>& textureMaterial
) {
    auto material = std::make_shared<PBRMaterial>(shader);
    
    if (textureMaterial) {
        PBRMaterialParams params;
        params.BaseColor = textureMaterial->BaseColor;
        params.Metallic = textureMaterial->MetallicFactor;
        params.Roughness = textureMaterial->RoughnessFactor;
        params.EmissiveColor = textureMaterial->EmissiveColor;
        // Use default values for parameters not in TextureMaterial
        params.Reflectance = 0.5f;
        params.AO = 1.0f;
        params.EmissiveFactor = 1.0f;
        params.NormalScale = 1.0f;
        
        material->SetParams(params);
    }
    
    return material;
}

void PBRMaterial::BindPBR() const {
    Bind();
    
    auto shader = GetShader();
    if (!shader) return;
    
    // Core parameters
    shader->setVec4("uBaseColor", params_.BaseColor);
    shader->setFloat("uMetallicFactor", params_.Metallic);
    shader->setFloat("uRoughnessFactor", params_.Roughness);
    shader->setFloat("uReflectance", params_.Reflectance);
    shader->setFloat("uAOFactor", params_.AO);
    shader->setVec3("uEmissiveColor", params_.EmissiveColor);
    shader->setFloat("uEmissiveFactor", params_.EmissiveFactor);
    shader->setFloat("uNormalScale", params_.NormalScale);
    
    // Clear Coat
    shader->setFloat("uClearCoat", params_.ClearCoat);
    shader->setFloat("uClearCoatRoughness", params_.ClearCoatRoughness);
    
    // Anisotropy
    shader->setFloat("uAnisotropy", params_.Anisotropy);
    shader->setVec3("uAnisotropyDirection", params_.AnisotropyDirection);
    
    // Sheen
    shader->setVec3("uSheenColor", params_.SheenColor);
    shader->setFloat("uSheenRoughness", params_.SheenRoughness);
    
    // Subsurface
    shader->setVec3("uSubsurfaceColor", params_.SubsurfaceColor);
    shader->setFloat("uSubsurfacePower", params_.SubsurfacePower);
    shader->setFloat("uThickness", params_.Thickness);
    
    // Transmission
    shader->setFloat("uTransmission", params_.Transmission);
    shader->setFloat("uIOR", params_.IOR);
}

void PBRMaterial::BindIBL(const IBLData& ibl) const {
    auto shader = GetShader();
    if (!shader) return;
    
    // Bind Spherical Harmonics coefficients
    shader->setVec3Array("uSH", ibl.SphericalHarmonics, 9);
    shader->setFloat("uIBLIntensity", ibl.Intensity);
    shader->setVec3("uSkyColor", ibl.SkyColor);
    shader->setVec3("uGroundColor", ibl.GroundColor);
}

}  // namespace se
