#include "engine/ui/native/font/UIFontManager.h"
#include "engine/ui/native/font/UIFont.h"
#include "engine/Log.h"

#include <ft2build.h>
#include FT_FREETYPE_H

namespace se::ui {

UIFontManager& UIFontManager::Get() {
    static UIFontManager instance;
    return instance;
}

UIFontManager::~UIFontManager() {
    Shutdown();
}

bool UIFontManager::Initialize() {
    SE_LOG_INFO("[UIFontManager] Initialize() called");
    if (initialized_) {
        SE_LOG_INFO("[UIFontManager] Already initialized");
        return true;
    }
    
    FT_Error error = FT_Init_FreeType(&ftLibrary_);
    if (error) {
        SE_LOG_ERROR("[UIFontManager] Failed to initialize FreeType: error {}", error);
        return false;
    }
    
    initialized_ = true;
    SE_LOG_INFO("[UIFontManager] FreeType initialized successfully!");
    return true;
}

void UIFontManager::Shutdown() {
    if (!initialized_) return;
    
    // Clear font cache (releases all fonts)
    fontCache_.clear();
    
    if (ftLibrary_) {
        FT_Done_FreeType(ftLibrary_);
        ftLibrary_ = nullptr;
    }
    
    initialized_ = false;
    SE_LOG_INFO("[UIFontManager] FreeType shutdown");
}

std::string UIFontManager::MakeCacheKey(const std::string& path, float size) const {
    return path + "|" + std::to_string(static_cast<int>(size * 10));
}

std::shared_ptr<UIFont> UIFontManager::LoadFont(const std::string& path, float fontSize) {
    
    if (!initialized_) {
        SE_LOG_ERROR("[UIFontManager] Cannot load font - not initialized");
        return nullptr;
    }
    
    std::string key = MakeCacheKey(path, fontSize);
    
    // Check cache
    auto it = fontCache_.find(key);
    if (it != fontCache_.end()) {
        return it->second;
    }
    
    // Load new font
    auto font = std::make_shared<UIFont>(path, fontSize);
    if (!font->Load()) {
        SE_LOG_ERROR("[UIFontManager] Failed to load font: {}", path);
        return nullptr;
    }
    
    fontCache_[key] = font;
    SE_LOG_INFO("[UIFontManager] Loaded font: {} at size {}", path, fontSize);
    
    return font;
}

std::shared_ptr<UIFont> UIFontManager::GetDefaultFont(float fontSize) {
    return LoadFont(defaultFontPath_, fontSize);
}

}  // namespace se::ui
