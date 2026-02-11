#include "engine/renderer/MaterialHandle.h"

#include <filament/MaterialInstance.h>
#include <filament/Material.h>

#include <math/vec3.h>
#include <math/vec4.h>

namespace se {

void MaterialHandle::SetColor(float r, float g, float b, float a) {
    if (!instance_) return;
    instance_->setParameter("baseColor", filament::math::float4{r, g, b, a});
}

void MaterialHandle::SetMetallic(float metallic) {
    if (!instance_) return;
    instance_->setParameter("metallic", metallic);
}

void MaterialHandle::SetRoughness(float roughness) {
    if (!instance_) return;
    instance_->setParameter("roughness", roughness);
}

void MaterialHandle::SetReflectance(float reflectance) {
    if (!instance_) return;
    instance_->setParameter("reflectance", reflectance);
}

void MaterialHandle::SetEmissive(float r, float g, float b, float intensity) {
    if (!instance_) return;
    instance_->setParameter("emissive", filament::math::float4{r * intensity, g * intensity, b * intensity, 1.0f});
}

} // namespace se
