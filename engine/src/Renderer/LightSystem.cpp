#include "engine/renderer/LightSystem.h"

#include <filament/Engine.h>
#include <filament/Scene.h>
#include <filament/LightManager.h>

#include <utils/EntityManager.h>

#include <math/vec3.h>

#include <spdlog/spdlog.h>

namespace se {

LightSystem::~LightSystem() {
    Shutdown();
}

void LightSystem::Init(filament::Engine* engine, filament::Scene* scene) {
    if (engine_) {
        spdlog::warn("LightSystem::Init called but already initialized. Ignoring.");
        return;
    }
    engine_ = engine;
    scene_  = scene;
    spdlog::info("LightSystem initialized.");
}

void LightSystem::Shutdown() {
    if (!engine_) return;

    ClearLights();

    engine_ = nullptr;
    scene_  = nullptr;
    spdlog::info("LightSystem shut down.");
}

void LightSystem::SetDirectionalLight(float dirX, float dirY, float dirZ,
                                       float r, float g, float b,
                                       float intensity, bool castShadows) {
    if (!engine_ || !scene_) return;

    // Remove existing directional light
    if (directional_light_) {
        scene_->remove(*directional_light_);
        engine_->destroy(*directional_light_);
        utils::EntityManager::get().destroy(*directional_light_);
        delete directional_light_;
        directional_light_ = nullptr;
    }

    auto& em = utils::EntityManager::get();
    auto entity = em.create();

    filament::LightManager::Builder(filament::LightManager::Type::SUN)
        .color({r, g, b})
        .intensity(intensity)
        .direction({dirX, dirY, dirZ})
        .castShadows(castShadows)
        .sunAngularRadius(1.9f)   // Realistic sun size
        .sunHaloSize(10.0f)
        .sunHaloFalloff(80.0f)
        .build(*engine_, entity);

    scene_->addEntity(entity);
    directional_light_ = new utils::Entity(entity);

    spdlog::debug("Directional light set: dir=({},{},{}), color=({},{},{}), intensity={}",
                  dirX, dirY, dirZ, r, g, b, intensity);
}

size_t LightSystem::AddPointLight(float posX, float posY, float posZ,
                                   float r, float g, float b,
                                   float intensity, float falloff) {
    if (!engine_ || !scene_) return 0;

    auto& em = utils::EntityManager::get();
    auto entity = em.create();

    filament::LightManager::Builder(filament::LightManager::Type::POINT)
        .color({r, g, b})
        .intensity(intensity)
        .falloff(falloff)
        .position({posX, posY, posZ})
        .build(*engine_, entity);

    scene_->addEntity(entity);

    auto* entityPtr = new utils::Entity(entity);
    point_lights_.push_back(entityPtr);

    spdlog::debug("Point light added at ({},{},{}), intensity={}", posX, posY, posZ, intensity);

    return point_lights_.size() - 1;
}

void LightSystem::ClearLights() {
    if (!engine_ || !scene_) return;

    // Destroy directional light
    if (directional_light_) {
        scene_->remove(*directional_light_);
        engine_->destroy(*directional_light_);
        utils::EntityManager::get().destroy(*directional_light_);
        delete directional_light_;
        directional_light_ = nullptr;
    }

    // Destroy point lights
    for (auto* entity : point_lights_) {
        if (entity) {
            scene_->remove(*entity);
            engine_->destroy(*entity);
            utils::EntityManager::get().destroy(*entity);
            delete entity;
        }
    }
    point_lights_.clear();
}

} // namespace se
