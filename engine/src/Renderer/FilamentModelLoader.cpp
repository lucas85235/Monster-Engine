#include "engine/renderer/FilamentModelLoader.h"

#include <filament/Engine.h>
#include <filament/Scene.h>
#include <filament/TransformManager.h>

#include <gltfio/AssetLoader.h>
#include <gltfio/FilamentAsset.h>
#include <gltfio/FilamentInstance.h>
#include <gltfio/MaterialProvider.h>
#include <gltfio/ResourceLoader.h>
#include <gltfio/TextureProvider.h>
#include <gltfio/Animator.h>
#include <gltfio/materials/uberarchive.h>

#include <utils/Entity.h>
#include <utils/EntityManager.h>

#include <math/mat4.h>

#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <vector>
#include <cstring>

namespace se {

// ─── ModelHandle implementation ─────────────────────────────────────────────

utils::Entity ModelHandle::GetRoot() const {
    if (!asset_) return utils::Entity();
    return asset_->getRoot();
}

size_t ModelHandle::GetEntityCount() const {
    if (!asset_) return 0;
    return asset_->getEntityCount();
}

filament::gltfio::Animator* ModelHandle::GetAnimator() const {
    if (!asset_) return nullptr;
    auto* instance = asset_->getInstance();
    if (!instance) return nullptr;
    return instance->getAnimator();
}

// ─── FilamentModelLoader ────────────────────────────────────────────────────

FilamentModelLoader::~FilamentModelLoader() {
    Shutdown();
}

void FilamentModelLoader::Init(filament::Engine* engine, filament::Scene* scene) {
    if (engine_) {
        spdlog::warn("FilamentModelLoader::Init called but already initialized. Ignoring.");
        return;
    }

    engine_ = engine;
    scene_  = scene;

    // Create UbershaderProvider using precompiled PBR materials from uberarchive
    material_provider_ = filament::gltfio::createUbershaderProvider(
        engine_, UBERARCHIVE_DEFAULT_DATA, UBERARCHIVE_DEFAULT_SIZE);

    if (!material_provider_) {
        spdlog::critical("FilamentModelLoader: Failed to create UbershaderProvider!");
        return;
    }

    // Create AssetLoader
    filament::gltfio::AssetConfiguration config;
    config.engine    = engine_;
    config.materials = material_provider_;
    asset_loader_ = filament::gltfio::AssetLoader::create(config);

    if (!asset_loader_) {
        spdlog::critical("FilamentModelLoader: Failed to create AssetLoader!");
        return;
    }

    // Create ResourceLoader for uploading buffers and textures to GPU
    filament::gltfio::ResourceConfiguration resConfig;
    resConfig.engine                   = engine_;
    resConfig.gltfPath                 = nullptr; // Set per-load
    resConfig.normalizeSkinningWeights = true;
    resource_loader_ = new filament::gltfio::ResourceLoader(resConfig);

    // Create StbProvider for decoding PNG/JPEG textures embedded in glTF
    texture_provider_ = filament::gltfio::createStbProvider(engine_);
    resource_loader_->addTextureProvider("image/png", texture_provider_);
    resource_loader_->addTextureProvider("image/jpeg", texture_provider_);

    spdlog::info("FilamentModelLoader initialized with UbershaderProvider.");
}

void FilamentModelLoader::Shutdown() {
    if (!engine_) return;

    // Destroy all loaded assets
    for (auto* asset : assets_) {
        if (asset) {
            scene_->removeEntities(asset->getEntities(), asset->getEntityCount());
            asset_loader_->destroyAsset(asset);
        }
    }
    assets_.clear();
    animation_times_.clear();

    // Destroy providers and loaders
    if (resource_loader_) {
        delete resource_loader_;
        resource_loader_ = nullptr;
    }

    if (texture_provider_) {
        delete texture_provider_;
        texture_provider_ = nullptr;
    }

    if (material_provider_) {
        material_provider_->destroyMaterials();
        delete material_provider_;
        material_provider_ = nullptr;
    }

    if (asset_loader_) {
        filament::gltfio::AssetLoader::destroy(&asset_loader_);
        asset_loader_ = nullptr;
    }

    engine_ = nullptr;
    scene_  = nullptr;
    spdlog::info("FilamentModelLoader shut down.");
}

ModelHandle FilamentModelLoader::LoadModel(const std::string& path) {
    if (!engine_ || !asset_loader_) {
        spdlog::error("FilamentModelLoader::LoadModel called before Init()!");
        return ModelHandle();
    }

    // Validate file exists
    if (!std::filesystem::exists(path)) {
        spdlog::error("FilamentModelLoader: File not found: {}", path);
        return ModelHandle();
    }

    // Read the entire file into memory
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        spdlog::error("FilamentModelLoader: Failed to open file: {}", path);
        return ModelHandle();
    }

    auto fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
        spdlog::error("FilamentModelLoader: Failed to read file: {}", path);
        return ModelHandle();
    }
    file.close();

    // Parse the glTF/GLB content and create Filament entities
    filament::gltfio::FilamentAsset* asset =
        asset_loader_->createAsset(buffer.data(), static_cast<uint32_t>(buffer.size()));

    if (!asset) {
        spdlog::error("FilamentModelLoader: Failed to parse glTF asset: {}", path);
        return ModelHandle();
    }

    // Update ResourceLoader with the file path for resolving relative URIs
    std::string dirPath = std::filesystem::path(path).parent_path().string();
    filament::gltfio::ResourceConfiguration resConfig;
    resConfig.engine                   = engine_;
    resConfig.gltfPath                 = dirPath.c_str();
    resConfig.normalizeSkinningWeights = true;
    resource_loader_->setConfiguration(resConfig);

    // Load resources (textures, vertex buffers) synchronously
    if (!resource_loader_->loadResources(asset)) {
        spdlog::warn("FilamentModelLoader: Some resources may not have loaded for: {}", path);
    }

    // Release CPU-side source data (no longer needed after GPU upload)
    asset->releaseSourceData();

    // Add all renderable entities to the Filament scene
    scene_->addEntities(asset->getEntities(), asset->getEntityCount());

    assets_.push_back(asset);
    animation_times_[asset] = 0.0f;

    spdlog::info("FilamentModelLoader: Loaded model '{}' ({} entities, {} renderables)",
                 std::filesystem::path(path).filename().string(),
                 asset->getEntityCount(),
                 asset->getRenderableEntityCount());

    return ModelHandle(asset);
}

void FilamentModelLoader::DestroyModel(ModelHandle& handle) {
    if (!handle.IsValid() || !engine_ || !asset_loader_) return;

    auto* asset = handle.GetNative();

    // Remove from scene
    scene_->removeEntities(asset->getEntities(), asset->getEntityCount());

    // Remove from our tracking list
    auto it = std::find(assets_.begin(), assets_.end(), asset);
    if (it != assets_.end()) {
        assets_.erase(it);
    }
    animation_times_.erase(asset);

    // Destroy the asset
    asset_loader_->destroyAsset(asset);

    handle = ModelHandle();
}

void FilamentModelLoader::SetTransform(const ModelHandle& handle, const float* transform) {
    if (!handle.IsValid() || !engine_) return;

    auto& tcm = engine_->getTransformManager();
    auto root = handle.GetRoot();
    auto ti = tcm.getInstance(root);

    filament::math::mat4f mat;
    std::memcpy(&mat, transform, sizeof(float) * 16);
    tcm.setTransform(ti, mat);
}

void FilamentModelLoader::UpdateAnimations(float deltaTime) {
    for (auto* asset : assets_) {
        if (!asset) continue;
        auto* instance = asset->getInstance();
        if (!instance) continue;
        auto* animator = instance->getAnimator();
        if (!animator) continue;

        // If the asset has animations, apply the first one
        if (animator->getAnimationCount() > 0) {
            float& elapsed = animation_times_[asset];
            elapsed += deltaTime;
            animator->applyAnimation(0, elapsed);
            animator->updateBoneMatrices();
        }
    }
}

} // namespace se
