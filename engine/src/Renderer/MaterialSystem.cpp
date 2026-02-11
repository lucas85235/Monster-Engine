#include "engine/renderer/MaterialSystem.h"

#include <filament/Engine.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>

#include <filamat/MaterialBuilder.h>

#include <spdlog/spdlog.h>

#include <math/vec4.h>

namespace se {

// Standard PBR lit material shader (Filament shading model = lit)
static constexpr const char* kLitMaterialSource = R"FILAMENT(
void material(inout MaterialInputs material) {
    prepareMaterial(material);
    material.baseColor = materialParams.baseColor;
    material.metallic  = materialParams.metallic;
    material.roughness = materialParams.roughness;
    material.reflectance = materialParams.reflectance;
    material.emissive  = materialParams.emissive;
}
)FILAMENT";

// Unlit material shader (Filament shading model = unlit)
static constexpr const char* kUnlitMaterialSource = R"FILAMENT(
void material(inout MaterialInputs material) {
    prepareMaterial(material);
    material.baseColor = materialParams.baseColor;
}
)FILAMENT";

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
    // --- Build lit PBR material ---
    {
        filamat::MaterialBuilder builder;
        builder
            .name("DefaultLit")
            .material(kLitMaterialSource)
            .shading(filament::Shading::LIT)
            .parameter("baseColor", filamat::MaterialBuilder::UniformType::FLOAT4)
            .parameter("metallic", filamat::MaterialBuilder::UniformType::FLOAT)
            .parameter("roughness", filamat::MaterialBuilder::UniformType::FLOAT)
            .parameter("reflectance", filamat::MaterialBuilder::UniformType::FLOAT)
            .parameter("emissive", filamat::MaterialBuilder::UniformType::FLOAT4)
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

        // Create default instance
        auto* instance = lit_material_->createInstance("DefaultLitInstance");
        instance->setParameter("baseColor", filament::math::float4{0.8f, 0.8f, 0.8f, 1.0f});
        instance->setParameter("metallic", 0.0f);
        instance->setParameter("roughness", 0.5f);
        instance->setParameter("reflectance", 0.5f);
        instance->setParameter("emissive", filament::math::float4{0.0f, 0.0f, 0.0f, 0.0f});

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

    instances_.push_back(instance);
    return MaterialHandle(instance);
}

MaterialHandle MaterialSystem::GetDefaultLit() {
    return default_lit_;
}

MaterialHandle MaterialSystem::GetDefaultUnlit() {
    return default_unlit_;
}

} // namespace se
