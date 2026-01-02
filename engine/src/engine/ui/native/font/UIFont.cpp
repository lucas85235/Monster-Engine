#include "engine/ui/native/font/UIFont.h"
#include "engine/ui/native/font/UIFontManager.h"
#include "engine/Log.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glad/glad.h>
#include <algorithm>
#include <vector>

namespace se::ui {

UIFont::UIFont(const std::string& path, float fontSize)
    : fontPath_(path), fontSize_(fontSize) {
}

UIFont::~UIFont() {
    Unload();
}

bool UIFont::Load() {
    if (loaded_) return true;
    
    auto& manager = UIFontManager::Get();
    FT_Library ftLib = manager.GetFTLibrary();
    
    if (!ftLib) {
        SE_LOG_ERROR("[UIFont] FreeType not initialized");
        return false;
    }
    
    FT_Error error = FT_New_Face(ftLib, fontPath_.c_str(), 0, &ftFace_);
    if (error == FT_Err_Unknown_File_Format) {
        SE_LOG_ERROR("[UIFont] Unsupported font format: {}", fontPath_);
        return false;
    } else if (error) {
        SE_LOG_ERROR("[UIFont] Failed to load font file: {} (error {})", fontPath_, error);
        return false;
    }
    
    // Set pixel size
    error = FT_Set_Pixel_Sizes(ftFace_, 0, static_cast<FT_UInt>(fontSize_));
    if (error) {
        SE_LOG_ERROR("[UIFont] Failed to set font size: {}", fontSize_);
        FT_Done_Face(ftFace_);
        ftFace_ = nullptr;
        return false;
    }
    
    // Get metrics
    lineHeight_ = static_cast<float>(ftFace_->size->metrics.height >> 6);
    ascender_ = static_cast<float>(ftFace_->size->metrics.ascender >> 6);
    descender_ = static_cast<float>(ftFace_->size->metrics.descender >> 6);
    
    // Create texture atlas
    CreateAtlasTexture();
    
    // Pre-load ASCII characters (32-126)
    for (uint32_t c = 32; c < 127; ++c) {
        GetGlyph(c);
    }
    
    loaded_ = true;
    SE_LOG_DEBUG("[UIFont] Loaded: {} at {}px (line height: {})", fontPath_, fontSize_, lineHeight_);
    
    return true;
}

void UIFont::Unload() {
    if (atlasTextureId_) {
        glDeleteTextures(1, &atlasTextureId_);
        atlasTextureId_ = 0;
    }
    
    if (ftFace_) {
        FT_Done_Face(ftFace_);
        ftFace_ = nullptr;
    }
    
    glyphCache_.clear();
    loaded_ = false;
}

void UIFont::CreateAtlasTexture() {
    glGenTextures(1, &atlasTextureId_);
    glBindTexture(GL_TEXTURE_2D, atlasTextureId_);
    
    // For single-channel texture
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    // Create texture and clear to zeros (black)
    std::vector<unsigned char> clearData(atlasWidth_ * atlasHeight_, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlasWidth_, atlasHeight_, 0, GL_RED, GL_UNSIGNED_BYTE, clearData.data());
    
    // Set parameters for clean text rendering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Start cursor with margin
    atlasCursorX_ = 1;
    atlasCursorY_ = 1;
    
    SE_LOG_DEBUG("[UIFont] Created atlas texture {}x{}", atlasWidth_, atlasHeight_);
}

const UIGlyph& UIFont::GetGlyph(uint32_t codepoint) {
    // Check cache
    auto it = glyphCache_.find(codepoint);
    if (it != glyphCache_.end()) {
        return it->second;
    }
    
    // Rasterize and cache
    UIGlyph glyph;
    if (RasterizeGlyph(codepoint, glyph)) {
        glyphCache_[codepoint] = glyph;
        return glyphCache_[codepoint];
    }
    
    return invalidGlyph_;
}

bool UIFont::RasterizeGlyph(uint32_t codepoint, UIGlyph& outGlyph) {
    if (!ftFace_) return false;
    
    FT_UInt glyphIndex = FT_Get_Char_Index(ftFace_, codepoint);
    
    FT_Error error = FT_Load_Glyph(ftFace_, glyphIndex, FT_LOAD_RENDER);
    if (error) {
        SE_LOG_WARN("[UIFont] Failed to load glyph for codepoint {}", codepoint);
        return false;
    }
    
    FT_GlyphSlot slot = ftFace_->glyph;
    FT_Bitmap& bitmap = slot->bitmap;
    
    outGlyph.width = static_cast<int>(bitmap.width);
    outGlyph.height = static_cast<int>(bitmap.rows);
    outGlyph.bearingX = slot->bitmap_left;
    outGlyph.bearingY = slot->bitmap_top;
    outGlyph.advance = static_cast<int>(slot->advance.x >> 6);
    
    // Check if glyph fits in current row (2px padding to prevent texture bleeding)
    if (atlasCursorX_ + outGlyph.width + 2 >= atlasWidth_) {
        // Move to next row
        atlasCursorX_ = 1;  // Start with 1px margin from edge
        atlasCursorY_ += atlasRowHeight_ + 2;
        atlasRowHeight_ = 0;
    }
    
    // Check if we need to expand atlas
    if (atlasCursorY_ + outGlyph.height + 2 >= atlasHeight_) {
        SE_LOG_WARN("[UIFont] Atlas full, glyph {} not added", codepoint);
        return false;
    }
    
    // Upload to atlas
    outGlyph.atlasX = atlasCursorX_;
    outGlyph.atlasY = atlasCursorY_;
    
    if (bitmap.buffer && outGlyph.width > 0 && outGlyph.height > 0) {
        UploadGlyphToAtlas(outGlyph, bitmap.buffer);
    }
    
    // Calculate UV coordinates
    outGlyph.u0 = static_cast<float>(outGlyph.atlasX) / atlasWidth_;
    outGlyph.v0 = static_cast<float>(outGlyph.atlasY) / atlasHeight_;
    outGlyph.u1 = static_cast<float>(outGlyph.atlasX + outGlyph.width) / atlasWidth_;
    outGlyph.v1 = static_cast<float>(outGlyph.atlasY + outGlyph.height) / atlasHeight_;
    
    outGlyph.valid = true;
    
    // Update cursor (2px padding)
    atlasCursorX_ += outGlyph.width + 2;
    atlasRowHeight_ = std::max(atlasRowHeight_, outGlyph.height);
    
    return true;
}

void UIFont::UploadGlyphToAtlas(const UIGlyph& glyph, const unsigned char* bitmap) {
    glBindTexture(GL_TEXTURE_2D, atlasTextureId_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    glTexSubImage2D(
        GL_TEXTURE_2D, 0,
        glyph.atlasX, glyph.atlasY,
        glyph.width, glyph.height,
        GL_RED, GL_UNSIGNED_BYTE,
        bitmap
    );
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

glm::vec2 UIFont::MeasureText(const std::string& text) {
    float width = 0.0f;
    float height = lineHeight_;
    
    for (char c : text) {
        // Use GetGlyph to ensure glyphs are loaded on demand
        const UIGlyph& glyph = GetGlyph(static_cast<uint32_t>(c));
        width += glyph.advance;
    }
    
    return {width, height};
}

}  // namespace se::ui
