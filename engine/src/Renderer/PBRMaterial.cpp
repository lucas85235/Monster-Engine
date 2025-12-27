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
    // First bind base material (shader)
    Bind();
    
    auto shader = GetShader();
    if (!shader) return;
    
    // Set PBR material parameters
    shader->setVec4("uBaseColor", params_.BaseColor);
    shader->setFloat("uMetallicFactor", params_.Metallic);
    shader->setFloat("uRoughnessFactor", params_.Roughness);
    shader->setFloat("uReflectance", params_.Reflectance);
    shader->setFloat("uAOFactor", params_.AO);
    shader->setVec3("uEmissiveColor", params_.EmissiveColor);
    shader->setFloat("uEmissiveFactor", params_.EmissiveFactor);
    shader->setFloat("uNormalScale", params_.NormalScale);
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
