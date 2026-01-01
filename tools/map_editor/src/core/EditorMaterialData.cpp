#include "core/EditorMaterialData.h"

#include "engine/assets/MaterialAssetBuilder.h"

namespace mst {

se::MaterialAsset EditorMaterialData::ToEngineAsset() const {
    se::MaterialAssetBuilder builder;
    
    builder.WithName(name)
           .WithUUID(uuid.empty() ? se::MaterialAssetBuilder::GenerateUUID() : uuid)
           .WithBaseColor(baseColor)
           .WithMetallic(metallic)
           .WithRoughness(roughness)
           .WithReflectance(reflectance)
           .WithAO(ao)
           .WithNormalScale(normalScale)
           .WithEmissive(emissiveColor, emissiveFactor)
           .WithClearCoat(clearCoat, clearCoatRoughness)
           .WithAnisotropy(anisotropy, anisotropyDirection)
           .WithSubsurface(subsurfaceColor, subsurfacePower, thickness)
           .WithTransmission(transmission, ior);
    
    if (useAlbedoTexture && !albedoTexturePath.empty()) {
        builder.WithAlbedoTexture(albedoTexturePath);
    }
    if (useNormalTexture && !normalTexturePath.empty()) {
        builder.WithNormalTexture(normalTexturePath);
    }
    if (useMetallicTexture && !metallicTexturePath.empty()) {
        builder.WithMetallicTexture(metallicTexturePath);
    }
    if (useRoughnessTexture && !roughnessTexturePath.empty()) {
        builder.WithRoughnessTexture(roughnessTexturePath);
    }
    if (useAOTexture && !aoTexturePath.empty()) {
        builder.WithAOTexture(aoTexturePath);
    }
    if (useEmissiveTexture && !emissiveTexturePath.empty()) {
        builder.WithEmissiveTexture(emissiveTexturePath);
    }
    
    return builder.Build();
}

void EditorMaterialData::FromEngineAsset(const se::MaterialAsset& asset) {
    name = asset.name;
    uuid = asset.uuid;
    
    const auto& def = asset.definition;
    baseColor = def.baseColor;
    metallic = def.metallic;
    roughness = def.roughness;
    reflectance = def.reflectance;
    ao = def.ao;
    normalScale = def.normalScale;
    
    emissiveColor = def.emissiveColor;
    emissiveFactor = def.emissiveFactor;
    
    clearCoat = def.clearCoat;
    clearCoatRoughness = def.clearCoatRoughness;
    
    anisotropy = def.anisotropy;
    anisotropyDirection = def.anisotropyDirection;
    
    sheenColor = def.sheenColor;
    sheenRoughness = def.sheenRoughness;
    
    subsurfaceColor = def.subsurfaceColor;
    subsurfacePower = def.subsurfacePower;
    thickness = def.thickness;
    
    transmission = def.transmission;
    ior = def.ior;
    
    albedoTexturePath = asset.albedoSourcePath;
    normalTexturePath = asset.normalSourcePath;
    metallicTexturePath = asset.metallicSourcePath;
    roughnessTexturePath = asset.roughnessSourcePath;
    aoTexturePath = asset.aoSourcePath;
    emissiveTexturePath = asset.emissiveSourcePath;
    
    useAlbedoTexture = !albedoTexturePath.empty() || (asset.albedoTexture && asset.albedoTexture->IsValid());
    useNormalTexture = !normalTexturePath.empty() || (asset.normalTexture && asset.normalTexture->IsValid());
    useMetallicTexture = !metallicTexturePath.empty() || (asset.metallicTexture && asset.metallicTexture->IsValid());
    useRoughnessTexture = !roughnessTexturePath.empty() || (asset.roughnessTexture && asset.roughnessTexture->IsValid());
    useAOTexture = !aoTexturePath.empty() || (asset.aoTexture && asset.aoTexture->IsValid());
    useEmissiveTexture = !emissiveTexturePath.empty() || (asset.emissiveTexture && asset.emissiveTexture->IsValid());
    
    isDirty = false;
}

}  // namespace mst
