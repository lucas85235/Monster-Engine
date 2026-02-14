#include "engine/renderer/MaterialSystem.h"

#include <filament/Engine.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>
#include <filament/Texture.h>
#include <filament/TextureSampler.h>

#include <filamat/MaterialBuilder.h>

#include <spdlog/spdlog.h>

#include <math/vec4.h>

#include <cstring>

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

static filament::Texture* createSolidTexture(filament::Engine* engine, uint8_t r, uint8_t g,
                                             uint8_t b, uint8_t a, bool sRGB) {
    if (!engine) return nullptr;

    auto* texture = filament::Texture::Builder()
                        .width(1)
                        .height(1)
                        .levels(1)
                        .sampler(filament::Texture::Sampler::SAMPLER_2D)
                        .format(sRGB ? filament::Texture::InternalFormat::SRGB8_A8
                                     : filament::Texture::InternalFormat::RGBA8)
                        .build(*engine);
    if (!texture) return nullptr;

    auto* pixel = new uint8_t[4];
    pixel[0] = r;
    pixel[1] = g;
    pixel[2] = b;
    pixel[3] = a;

    texture->setImage(*engine, 0,
                      filament::Texture::PixelBufferDescriptor(
                          pixel, 4, filament::Texture::Format::RGBA,
                          filament::Texture::Type::UBYTE,
                          [](void* data, size_t, void*) { delete[] static_cast<uint8_t*>(data); }));
    return texture;
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
    for (auto& slot : material_slots_) {
        if (slot.alive && slot.instance) {
            engine_->destroy(slot.instance);
            slot.instance = nullptr;
            slot.alive    = false;
        }
    }
    material_slots_.clear();
    free_slots_.clear();

    // Destroy base materials
    if (lit_material_) {
        engine_->destroy(lit_material_);
        lit_material_ = nullptr;
    }
    if (unlit_material_) {
        engine_->destroy(unlit_material_);
        unlit_material_ = nullptr;
    }
    if (fallback_white_texture_) {
        engine_->destroy(fallback_white_texture_);
        fallback_white_texture_ = nullptr;
    }
    if (fallback_normal_texture_) {
        engine_->destroy(fallback_normal_texture_);
        fallback_normal_texture_ = nullptr;
    }
    if (fallback_black_texture_) {
        engine_->destroy(fallback_black_texture_);
        fallback_black_texture_ = nullptr;
    }

    default_lit_   = MaterialHandle();
    default_unlit_ = MaterialHandle();

    filamat::MaterialBuilder::shutdown();

    engine_ = nullptr;
    spdlog::info("MaterialSystem shut down.");
}

void MaterialSystem::EnsureFallbackTextures() {
    if (!engine_) return;

    if (!fallback_white_texture_) {
        fallback_white_texture_ = createSolidTexture(engine_, 255, 255, 255, 255, true);
        if (!fallback_white_texture_) {
            spdlog::error("MaterialSystem: failed creating fallback white texture.");
        }
    }
    if (!fallback_normal_texture_) {
        fallback_normal_texture_ = createSolidTexture(engine_, 128, 128, 255, 255, false);
        if (!fallback_normal_texture_) {
            spdlog::error("MaterialSystem: failed creating fallback normal texture.");
        }
    }
    if (!fallback_black_texture_) {
        fallback_black_texture_ = createSolidTexture(engine_, 0, 0, 0, 255, false);
        if (!fallback_black_texture_) {
            spdlog::error("MaterialSystem: failed creating fallback black texture.");
        }
    }
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

        EnsureFallbackTextures();

        // Bind fallback textures as *material-level* defaults so that every
        // instance created from this material already has valid sampler state.
        // This eliminates Filament's per-frame "sampler parameters not set"
        // warnings which were the primary cause of the log-spam FPS drop.
        {
            auto sampler = getDefaultSampler();
            if (fallback_white_texture_)
                lit_material_->setDefaultParameter("baseColorMap", fallback_white_texture_, sampler);
            if (fallback_normal_texture_)
                lit_material_->setDefaultParameter("normalMap", fallback_normal_texture_, sampler);
            if (fallback_white_texture_)
                lit_material_->setDefaultParameter("metallicRoughnessMap", fallback_white_texture_, sampler);
            if (fallback_black_texture_)
                lit_material_->setDefaultParameter("aoMap", fallback_black_texture_, sampler);
        }

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
        auto sampler = getDefaultSampler();
        if (fallback_white_texture_) {
            instance->setParameter("baseColorMap", fallback_white_texture_, sampler);
        }
        if (fallback_normal_texture_) {
            instance->setParameter("normalMap", fallback_normal_texture_, sampler);
        }
        if (fallback_white_texture_) {
            instance->setParameter("metallicRoughnessMap", fallback_white_texture_, sampler);
        }
        if (fallback_black_texture_) {
            instance->setParameter("aoMap", fallback_black_texture_, sampler);
        }

        default_lit_ = AddInstance(instance);
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

        default_unlit_ = AddInstance(instance);
    }
}

MaterialHandle MaterialSystem::CreateMaterial(const MaterialConfig& config) {
    if (!lit_material_) {
        spdlog::error("MaterialSystem::CreateMaterial called before Init()!");
        return MaterialHandle();
    }
    EnsureFallbackTextures();

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

    filament::Texture* baseColorTex =
        hasBaseColor ? config.baseColorMap.GetNative() : fallback_white_texture_;
    filament::Texture* normalTex =
        hasNormal ? config.normalMap.GetNative() : fallback_normal_texture_;
    filament::Texture* mrTex =
        hasMR ? config.metallicRoughnessMap.GetNative() : fallback_white_texture_;
    filament::Texture* aoTex =
        hasAO ? config.aoMap.GetNative() : fallback_black_texture_;

    if (baseColorTex) {
        instance->setParameter("baseColorMap", baseColorTex, sampler);
    }
    if (normalTex) {
        instance->setParameter("normalMap", normalTex, sampler);
    }
    if (mrTex) {
        instance->setParameter("metallicRoughnessMap", mrTex, sampler);
    }
    if (aoTex) {
        instance->setParameter("aoMap", aoTex, sampler);
    }

    return AddInstance(instance);
}

MaterialHandle MaterialSystem::GetDefaultLit() {
    return default_lit_;
}

MaterialHandle MaterialSystem::GetDefaultUnlit() {
    return default_unlit_;
}

MaterialHandle MaterialSystem::AddInstance(filament::MaterialInstance* instance) {
    if (!instance) {
        return MaterialHandle();
    }

    uint32_t slotIndex = 0;
    if (!free_slots_.empty()) {
        slotIndex = free_slots_.back();
        free_slots_.pop_back();
    } else {
        slotIndex = static_cast<uint32_t>(material_slots_.size());
        material_slots_.emplace_back();
    }

    auto& slot   = material_slots_[slotIndex];
    slot.instance = instance;
    slot.alive    = true;

    return MaterialHandle(this, slotIndex, slot.generation);
}

filament::MaterialInstance* MaterialSystem::Resolve(const MaterialHandle& handle) const {
    if (!IsAlive(handle)) {
        return nullptr;
    }
    return material_slots_[handle.index_].instance;
}

bool MaterialSystem::IsAlive(const MaterialHandle& handle) const {
    if (handle.owner_ != this) {
        return false;
    }
    if (handle.index_ == MaterialHandle::kInvalidIndex) {
        return false;
    }
    if (handle.index_ >= material_slots_.size()) {
        return false;
    }

    const auto& slot = material_slots_[handle.index_];
    return slot.alive && slot.generation == handle.generation_;
}

// ─── MaterialHandle texture setters ─────────────────────────────────────────

void MaterialHandle::SetColor(float r, float g, float b, float a) {
    auto* instance = GetNative();
    if (!instance) return;
    instance->setParameter("baseColor", filament::math::float4{r, g, b, a});
}

void MaterialHandle::SetMetallic(float metallic) {
    auto* instance = GetNative();
    if (!instance) return;
    instance->setParameter("metallic", metallic);
}

void MaterialHandle::SetRoughness(float roughness) {
    auto* instance = GetNative();
    if (!instance) return;
    instance->setParameter("roughness", roughness);
}

void MaterialHandle::SetReflectance(float reflectance) {
    auto* instance = GetNative();
    if (!instance) return;
    instance->setParameter("reflectance", reflectance);
}

void MaterialHandle::SetEmissive(float r, float g, float b, float intensity) {
    auto* instance = GetNative();
    if (!instance) return;
    instance->setParameter("emissive", filament::math::float4{
        r * intensity, g * intensity, b * intensity, 0.0f});
}

void MaterialHandle::SetBaseColorMap(const TextureHandle& texture) {
    auto* instance = GetNative();
    if (!instance) return;
    if (texture.IsValid()) {
        instance->setParameter("baseColorMap", texture.GetNative(), getDefaultSampler());
        instance->setParameter("hasBaseColorMap", true);
    } else {
        if (owner_ && owner_->fallback_white_texture_) {
            instance->setParameter("baseColorMap", owner_->fallback_white_texture_, getDefaultSampler());
        }
        instance->setParameter("hasBaseColorMap", false);
    }
}

void MaterialHandle::SetNormalMap(const TextureHandle& texture) {
    auto* instance = GetNative();
    if (!instance) return;
    if (texture.IsValid()) {
        instance->setParameter("normalMap", texture.GetNative(), getDefaultSampler());
        instance->setParameter("hasNormalMap", true);
    } else {
        if (owner_ && owner_->fallback_normal_texture_) {
            instance->setParameter("normalMap", owner_->fallback_normal_texture_, getDefaultSampler());
        }
        instance->setParameter("hasNormalMap", false);
    }
}

void MaterialHandle::SetMetallicRoughnessMap(const TextureHandle& texture) {
    auto* instance = GetNative();
    if (!instance) return;
    if (texture.IsValid()) {
        instance->setParameter("metallicRoughnessMap", texture.GetNative(), getDefaultSampler());
        instance->setParameter("hasMetallicRoughnessMap", true);
    } else {
        if (owner_ && owner_->fallback_white_texture_) {
            instance->setParameter("metallicRoughnessMap", owner_->fallback_white_texture_, getDefaultSampler());
        }
        instance->setParameter("hasMetallicRoughnessMap", false);
    }
}

void MaterialHandle::SetAOMap(const TextureHandle& texture) {
    auto* instance = GetNative();
    if (!instance) return;
    if (texture.IsValid()) {
        instance->setParameter("aoMap", texture.GetNative(), getDefaultSampler());
        instance->setParameter("hasAOMap", true);
    } else {
        if (owner_ && owner_->fallback_black_texture_) {
            instance->setParameter("aoMap", owner_->fallback_black_texture_, getDefaultSampler());
        }
        instance->setParameter("hasAOMap", false);
    }
}

bool MaterialHandle::IsValid() const {
    return owner_ && owner_->IsAlive(*this);
}

filament::MaterialInstance* MaterialHandle::GetNative() const {
    if (!owner_) return nullptr;
    return owner_->Resolve(*this);
}

} // namespace se
