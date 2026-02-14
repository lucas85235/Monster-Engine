#include "engine/renderer/MaterialSystem.h"

#include <filament/Engine.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>
#include <filament/Texture.h>
#include <filament/TextureSampler.h>

#include <filamat/MaterialBuilder.h>

#include <spdlog/spdlog.h>

#include <math/vec4.h>

namespace se {

// ─── Textured PBR lit material shader ───────────────────────────────────────
// Supports optional texture maps via boolean flags.
// When a map is enabled, it overrides (or modulates) the uniform value.
static constexpr const char* kLitMaterialSource = R"FILAMENT(
void material(inout MaterialInputs material) {
    prepareMaterial(material);

    // Base color
    float4 color = materialParams.baseColor;
    if (materialParams.hasBaseColorMap) {
        color *= texture(materialParams_baseColorMap, getUV0());
    }
    material.baseColor = color;

    // Normal map
    if (materialParams.hasNormalMap) {
        float3 n = texture(materialParams_normalMap, getUV0()).xyz * 2.0 - 1.0;
        material.normal = n;
    }

    // Metallic / Roughness
    float metallic  = materialParams.metallic;
    float roughness = materialParams.roughness;
    if (materialParams.hasMetallicRoughnessMap) {
        float4 mr = texture(materialParams_metallicRoughnessMap, getUV0());
        roughness *= mr.g;
        metallic  *= mr.b;
    }
    material.metallic  = metallic;
    material.roughness = roughness;

    // AO
    if (materialParams.hasAOMap) {
        float ao = texture(materialParams_aoMap, getUV0()).r;
        material.ambientOcclusion = ao;
    }

    material.reflectance = materialParams.reflectance;
    material.emissive    = materialParams.emissive;
}
)FILAMENT";

// Unlit material shader (Filament shading model = unlit)
static constexpr const char* kUnlitMaterialSource = R"FILAMENT(
void material(inout MaterialInputs material) {
    prepareMaterial(material);
    material.baseColor = materialParams.baseColor;
}
)FILAMENT";

// Default texture sampler: trilinear filtering with repeat wrapping
static filament::TextureSampler getDefaultSampler() {
    filament::TextureSampler sampler(filament::TextureSampler::MinFilter::LINEAR_MIPMAP_LINEAR,
                                     filament::TextureSampler::MagFilter::LINEAR,
                                     filament::TextureSampler::WrapMode::REPEAT);
    return sampler;
}

MaterialSystem::~MaterialSystem() {
    Shutdown();
}

void MaterialSystem::Init(filament::Engine* engine) {
    if (engine_) {
        spdlog::warn("MaterialSystem::Init called but already initialized. Ignoring.");
        return;
    }

    engine_ = engine;

    // Initialize the material compiler
    filamat::MaterialBuilder::init();

    CreateBuiltInMaterials();

    spdlog::info("MaterialSystem initialized with built-in PBR materials.");
}

void MaterialSystem::Shutdown() {
    if (!engine_) return;

    // Destroy all material instances
    for (auto* instance : instances_) {
        engine_->destroy(instance);
    }
    instances_.clear();

    // Destroy base materials
    if (lit_material_) {
        engine_->destroy(lit_material_);
        lit_material_ = nullptr;
    }
    if (unlit_material_) {
        engine_->destroy(unlit_material_);
        unlit_material_ = nullptr;
    }

    default_lit_   = MaterialHandle();
    default_unlit_ = MaterialHandle();

    filamat::MaterialBuilder::shutdown();

    engine_ = nullptr;
    spdlog::info("MaterialSystem shut down.");
}

void MaterialSystem::CreateBuiltInMaterials() {
    using SamplerType = filamat::MaterialBuilder::SamplerType;
    using SamplerFormat = filamat::MaterialBuilder::SamplerFormat;

    // --- Build lit PBR material (with texture support) ---
    {
        filamat::MaterialBuilder builder;
        builder
            .name("DefaultLit")
            .material(kLitMaterialSource)
            .shading(filament::Shading::LIT)
            .require(filament::VertexAttribute::UV0)
            // Uniform parameters
            .parameter("baseColor",   filamat::MaterialBuilder::UniformType::FLOAT4)
            .parameter("metallic",    filamat::MaterialBuilder::UniformType::FLOAT)
            .parameter("roughness",   filamat::MaterialBuilder::UniformType::FLOAT)
            .parameter("reflectance", filamat::MaterialBuilder::UniformType::FLOAT)
            .parameter("emissive",    filamat::MaterialBuilder::UniformType::FLOAT4)
            // Boolean flags (as float: 0.0 = false, 1.0 = true — Filament uses bool)
            .parameter("hasBaseColorMap",          filamat::MaterialBuilder::UniformType::BOOL)
            .parameter("hasNormalMap",              filamat::MaterialBuilder::UniformType::BOOL)
            .parameter("hasMetallicRoughnessMap",   filamat::MaterialBuilder::UniformType::BOOL)
            .parameter("hasAOMap",                  filamat::MaterialBuilder::UniformType::BOOL)
            // Texture samplers
            .parameter("baseColorMap",          SamplerType::SAMPLER_2D, SamplerFormat::FLOAT)
            .parameter("normalMap",              SamplerType::SAMPLER_2D, SamplerFormat::FLOAT)
            .parameter("metallicRoughnessMap",   SamplerType::SAMPLER_2D, SamplerFormat::FLOAT)
            .parameter("aoMap",                  SamplerType::SAMPLER_2D, SamplerFormat::FLOAT)
            .targetApi(filamat::MaterialBuilder::TargetApi::ALL)
            .platform(filamat::MaterialBuilder::Platform::ALL);

        filamat::Package package = builder.build(engine_->getJobSystem());
        if (!package.isValid()) {
            spdlog::critical("Failed to build DefaultLit material!");
            return;
        }

        lit_material_ = filament::Material::Builder()
            .package(package.getData(), package.getSize())
            .build(*engine_);

        // Create default instance (no textures active)
        auto* instance = lit_material_->createInstance("DefaultLitInstance");
        instance->setParameter("baseColor", filament::math::float4{0.8f, 0.8f, 0.8f, 1.0f});
        instance->setParameter("metallic", 0.0f);
        instance->setParameter("roughness", 0.5f);
        instance->setParameter("reflectance", 0.5f);
        instance->setParameter("emissive", filament::math::float4{0.0f, 0.0f, 0.0f, 0.0f});
        instance->setParameter("hasBaseColorMap", false);
        instance->setParameter("hasNormalMap", false);
        instance->setParameter("hasMetallicRoughnessMap", false);
        instance->setParameter("hasAOMap", false);

        instances_.push_back(instance);
        default_lit_ = MaterialHandle(instance);
    }

    // --- Build unlit material ---
    {
        filamat::MaterialBuilder builder;
        builder
            .name("DefaultUnlit")
            .material(kUnlitMaterialSource)
            .shading(filament::Shading::UNLIT)
            .parameter("baseColor", filamat::MaterialBuilder::UniformType::FLOAT4)
            .targetApi(filamat::MaterialBuilder::TargetApi::ALL)
            .platform(filamat::MaterialBuilder::Platform::ALL);

        filamat::Package package = builder.build(engine_->getJobSystem());
        if (!package.isValid()) {
            spdlog::critical("Failed to build DefaultUnlit material!");
            return;
        }

        unlit_material_ = filament::Material::Builder()
            .package(package.getData(), package.getSize())
            .build(*engine_);

        auto* instance = unlit_material_->createInstance("DefaultUnlitInstance");
        instance->setParameter("baseColor", filament::math::float4{1.0f, 1.0f, 1.0f, 1.0f});

        instances_.push_back(instance);
        default_unlit_ = MaterialHandle(instance);
    }
}

MaterialHandle MaterialSystem::CreateMaterial(const MaterialConfig& config) {
    if (!lit_material_) {
        spdlog::error("MaterialSystem::CreateMaterial called before Init()!");
        return MaterialHandle();
    }

    auto* instance = lit_material_->createInstance();
    instance->setParameter("baseColor", filament::math::float4{
        config.baseColor[0], config.baseColor[1],
        config.baseColor[2], config.baseColor[3]
    });
    instance->setParameter("metallic",    config.metallic);
    instance->setParameter("roughness",   config.roughness);
    instance->setParameter("reflectance", config.reflectance);
    instance->setParameter("emissive", filament::math::float4{
        config.emissive[0] * config.emissiveIntensity,
        config.emissive[1] * config.emissiveIntensity,
        config.emissive[2] * config.emissiveIntensity,
        0.0f
    });

    // Set texture maps if provided
    auto sampler = getDefaultSampler();

    bool hasBaseColor = config.baseColorMap.IsValid();
    bool hasNormal    = config.normalMap.IsValid();
    bool hasMR        = config.metallicRoughnessMap.IsValid();
    bool hasAO        = config.aoMap.IsValid();

    instance->setParameter("hasBaseColorMap",        hasBaseColor);
    instance->setParameter("hasNormalMap",            hasNormal);
    instance->setParameter("hasMetallicRoughnessMap", hasMR);
    instance->setParameter("hasAOMap",                hasAO);

    if (hasBaseColor) {
        instance->setParameter("baseColorMap", config.baseColorMap.GetNative(), sampler);
    }
    if (hasNormal) {
        instance->setParameter("normalMap", config.normalMap.GetNative(), sampler);
    }
    if (hasMR) {
        instance->setParameter("metallicRoughnessMap", config.metallicRoughnessMap.GetNative(), sampler);
    }
    if (hasAO) {
        instance->setParameter("aoMap", config.aoMap.GetNative(), sampler);
    }

    instances_.push_back(instance);
    return MaterialHandle(instance);
}

MaterialHandle MaterialSystem::GetDefaultLit() {
    return default_lit_;
}

MaterialHandle MaterialSystem::GetDefaultUnlit() {
    return default_unlit_;
}

// ─── MaterialHandle texture setters ─────────────────────────────────────────

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
    instance_->setParameter("emissive", filament::math::float4{
        r * intensity, g * intensity, b * intensity, 0.0f});
}

void MaterialHandle::SetBaseColorMap(const TextureHandle& texture) {
    if (!instance_) return;
    if (texture.IsValid()) {
        instance_->setParameter("baseColorMap", texture.GetNative(), getDefaultSampler());
        instance_->setParameter("hasBaseColorMap", true);
    } else {
        instance_->setParameter("hasBaseColorMap", false);
    }
}

void MaterialHandle::SetNormalMap(const TextureHandle& texture) {
    if (!instance_) return;
    if (texture.IsValid()) {
        instance_->setParameter("normalMap", texture.GetNative(), getDefaultSampler());
        instance_->setParameter("hasNormalMap", true);
    } else {
        instance_->setParameter("hasNormalMap", false);
    }
}

void MaterialHandle::SetMetallicRoughnessMap(const TextureHandle& texture) {
    if (!instance_) return;
    if (texture.IsValid()) {
        instance_->setParameter("metallicRoughnessMap", texture.GetNative(), getDefaultSampler());
        instance_->setParameter("hasMetallicRoughnessMap", true);
    } else {
        instance_->setParameter("hasMetallicRoughnessMap", false);
    }
}

void MaterialHandle::SetAOMap(const TextureHandle& texture) {
    if (!instance_) return;
    if (texture.IsValid()) {
        instance_->setParameter("aoMap", texture.GetNative(), getDefaultSampler());
        instance_->setParameter("hasAOMap", true);
    } else {
        instance_->setParameter("hasAOMap", false);
    }
}

} // namespace se
