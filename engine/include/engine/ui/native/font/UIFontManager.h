#pragma once

#include <se_pch.h>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <memory>

// Forward declare FreeType types
typedef struct FT_LibraryRec_* FT_Library;

namespace se::ui {

class UIFont;

/**
 * @class UIFontManager
 * @brief Singleton managing font loading and caching
 *
 * Uses FreeType for glyph rasterization. Caches fonts by path+size.
 */
class UIFontManager {
public:
    static UIFontManager& Get();
    
    // Lifecycle
    bool Initialize();
    void Shutdown();
    bool IsInitialized() const { return initialized_; }
    
    // Font loading
    std::shared_ptr<UIFont> LoadFont(const std::string& path, float fontSize);
    std::shared_ptr<UIFont> GetDefaultFont(float fontSize = 16.0f);
    
    // Set default font path
    void SetDefaultFontPath(const std::string& path) { defaultFontPath_ = path; }
    
    // FreeType access
    FT_Library GetFTLibrary() const { return ftLibrary_; }
    
private:
    UIFontManager() = default;
    ~UIFontManager();
    
    UIFontManager(const UIFontManager&) = delete;
    UIFontManager& operator=(const UIFontManager&) = delete;
    
    std::string MakeCacheKey(const std::string& path, float size) const;
    
private:
    FT_Library ftLibrary_ = nullptr;
    bool initialized_ = false;
    
    std::string defaultFontPath_ = "assets/fonts/Roboto-Regular.ttf";
    
    // Cache: "path|size" -> Font
    std::unordered_map<std::string, std::shared_ptr<UIFont>> fontCache_;
};

}  // namespace se::ui
