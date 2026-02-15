#include "engine/resources/TextureManager.h"

#include <filesystem>

#include "engine/Log.h"
#include "engine/renderer/Texture.h"

namespace se {

std::unordered_map<std::string, std::shared_ptr<Texture>> TextureManager::cache_;
bool TextureManager::initialized_ = false;

void TextureManager::Init() {
    if (initialized_) {
        SE_LOG_WARN("TextureManager already initialized");
        return;
    }

    SE_LOG_INFO("Initializing TextureManager");
    cache_.clear();
    initialized_ = true;
}

void TextureManager::Shutdown() {
    if (!initialized_) return;

    SE_LOG_INFO("Shutting down TextureManager");
    ClearCache();
    initialized_ = false;
}

std::shared_ptr<Texture> TextureManager::Load(const std::string& path) {
    if (!initialized_) {
        SE_LOG_ERROR("TextureManager not initialized!");
        return nullptr;
    }

    if (path.empty()) {
        return nullptr;
    }

    std::filesystem::path filePath(path);
    std::string name = filePath.stem().string();

    auto it = cache_.find(name);
    if (it != cache_.end()) {
        SE_LOG_INFO("TextureManager: Texture '{}' found in cache", name);
        return it->second;
    }

    if (!FileExists(path)) {
        SE_LOG_WARN("TextureManager: Texture file not found '{}'", path);
        return nullptr;
    }

    SE_LOG_INFO("TextureManager: Loading texture from '{}'", path);

    auto texture = Texture::Create(path);
    if (!texture) {
        SE_LOG_ERROR("TextureManager: Failed to create texture from '{}'", path);
        return nullptr;
    }

    cache_[name] = texture;
    SE_LOG_INFO("TextureManager: Texture '{}' loaded and cached", name);

    return texture;
}

std::shared_ptr<Texture> TextureManager::Get(const std::string& name) {
    auto it = cache_.find(name);
    if (it != cache_.end()) {
        return it->second;
    }
    return nullptr;
}

bool TextureManager::Has(const std::string& name) {
    return cache_.find(name) != cache_.end();
}

bool TextureManager::FileExists(const std::string& path) {
    return std::filesystem::exists(path);
}

void TextureManager::Unload(const std::string& name) {
    auto it = cache_.find(name);
    if (it != cache_.end()) {
        SE_LOG_INFO("TextureManager: Unloading texture '{}'", name);
        cache_.erase(it);
    }
}

void TextureManager::ClearCache() {
    SE_LOG_INFO("TextureManager: Clearing cache ({} textures)", cache_.size());
    cache_.clear();
}

}  // namespace se
