#pragma once

#include "engine/ui/native/font/UIGlyph.h"

#include <string>
#include <unordered_map>
#include <cstdint>

// Forward declare FreeType types
typedef struct FT_FaceRec_* FT_Face;

namespace se::ui {

/**
 * @class UIFont
 * @brief A loaded font at a specific size with cached glyphs
 *
 * Manages a texture atlas for glyph storage and provides
 * glyph lookup with on-demand rasterization.
 */
class UIFont {
public:
    UIFont(const std::string& path, float fontSize);
    ~UIFont();
    
    bool Load();
    void Unload();
    bool IsLoaded() const { return loaded_; }
    
    // Glyph access (lazy-loads if not cached)
    const UIGlyph& GetGlyph(uint32_t codepoint);
    
    // Metrics
    float GetFontSize() const { return fontSize_; }
    float GetLineHeight() const { return lineHeight_; }
    float GetAscender() const { return ascender_; }
    float GetDescender() const { return descender_; }
    
    // Atlas texture
    uint32_t GetAtlasTextureId() const { return atlasTextureId_; }
    int GetAtlasWidth() const { return atlasWidth_; }
    int GetAtlasHeight() const { return atlasHeight_; }
    
    // Text measurement (loads glyphs on demand)
    glm::vec2 MeasureText(const std::string& text);
    
private:
    bool RasterizeGlyph(uint32_t codepoint, UIGlyph& outGlyph);
    void CreateAtlasTexture();
    void ExpandAtlas();
    void UploadGlyphToAtlas(const UIGlyph& glyph, const unsigned char* bitmap);
    
private:
    std::string fontPath_;
    float fontSize_ = 16.0f;
    
    FT_Face ftFace_ = nullptr;
    bool loaded_ = false;
    
    // Font metrics
    float lineHeight_ = 0.0f;
    float ascender_ = 0.0f;
    float descender_ = 0.0f;
    
    // Glyph cache
    std::unordered_map<uint32_t, UIGlyph> glyphCache_;
    UIGlyph invalidGlyph_;
    
    // Texture atlas
    uint32_t atlasTextureId_ = 0;
    int atlasWidth_ = 512;
    int atlasHeight_ = 512;
    int atlasCursorX_ = 0;
    int atlasCursorY_ = 0;
    int atlasRowHeight_ = 0;
};

}  // namespace se::ui
