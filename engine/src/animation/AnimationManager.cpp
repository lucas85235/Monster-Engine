#include "engine/animation/AnimationManager.h"

#include <filesystem>

#include "engine/animation/AnimationClip.h"
#include "engine/animation/AnimationLoader.h"
#include "engine/Log.h"

namespace se {

std::unordered_map<std::string, std::shared_ptr<AnimationClip>> AnimationManager::cache_;
bool AnimationManager::initialized_ = false;

void AnimationManager::Init() {
    if (initialized_) return;
    initialized_ = true;
    SE_LOG_INFO("AnimationManager: Initialized");
}

void AnimationManager::Shutdown() {
    if (!initialized_) return;
    ClearCache();
    initialized_ = false;
    SE_LOG_INFO("AnimationManager: Shutdown");
}

std::shared_ptr<AnimationClip> AnimationManager::Load(const std::string& path) {
    if (!initialized_) Init();
    
    std::filesystem::path filePath(path);
    std::string name = filePath.stem().string();
    
    auto it = cache_.find(name);
    if (it != cache_.end()) {
        SE_LOG_INFO("AnimationManager: Cache hit for '{}'", name);
        return it->second;
    }
    
    auto clip = AnimationLoader::Load(path);
    if (!clip) {
        SE_LOG_ERROR("AnimationManager: Failed to load '{}'", path);
        return nullptr;
    }
    
    auto sharedClip = std::shared_ptr<AnimationClip>(std::move(clip));
    cache_[name] = sharedClip;
    
    SE_LOG_INFO("AnimationManager: Loaded and cached '{}'", name);
    return sharedClip;
}

std::shared_ptr<AnimationClip> AnimationManager::Get(const std::string& name) {
    auto it = cache_.find(name);
    if (it != cache_.end()) {
        return it->second;
    }
    return nullptr;
}

bool AnimationManager::Has(const std::string& name) {
    return cache_.find(name) != cache_.end();
}

void AnimationManager::Unload(const std::string& name) {
    auto it = cache_.find(name);
    if (it != cache_.end()) {
        cache_.erase(it);
        SE_LOG_INFO("AnimationManager: Unloaded '{}'", name);
    }
}

void AnimationManager::ClearCache() {
    size_t count = cache_.size();
    cache_.clear();
    SE_LOG_INFO("AnimationManager: Cleared {} cached animations", count);
}

}  // namespace se
