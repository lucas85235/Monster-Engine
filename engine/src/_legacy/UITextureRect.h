#pragma once

#include "engine/ui/native/UIControl.h"

namespace se::ui {

/**
 * @class UITextureRect
 * @brief Widget that displays a texture/image.
 *
 * Features:
 * - Texture display with various stretch modes
 * - UV rect for texture atlas support
 * - Modulate color tinting
 */
class UITextureRect : public UIControl {
public:
    enum class StretchMode {
        SCALE,           // Scale to fill, ignoring aspect ratio
        KEEP,            // Keep original size (may clip or have empty space)
        KEEP_CENTERED,   // Keep original size, centered
        KEEP_ASPECT,     // Scale keeping aspect ratio (may have bars)
        KEEP_ASPECT_COVERED,  // Scale keeping aspect ratio (may crop)
        KEEP_ASPECT_CENTERED  // Scale keeping aspect ratio, centered
    };
    
    UITextureRect();
    ~UITextureRect() override = default;
    
    // Texture
    void SetTexture(uint32_t textureId) { textureId_ = textureId; QueueRedraw(); }
    uint32_t GetTexture() const { return textureId_; }
    
    // Original texture size (for aspect ratio calculations)
    void SetTextureSize(const glm::vec2& size) { textureSize_ = size; UpdateMinimumSize(); QueueRedraw(); }
    glm::vec2 GetTextureSize() const { return textureSize_; }
    
    // UV rect for texture atlas
    void SetUVRect(const glm::vec4& uvRect) { uvRect_ = uvRect; QueueRedraw(); }
    glm::vec4 GetUVRect() const { return uvRect_; }
    
    // Stretch mode
    void SetStretchMode(StretchMode mode) { stretchMode_ = mode; QueueRedraw(); }
    StretchMode GetStretchMode() const { return stretchMode_; }
    
    // Modulate color
    void SetModulate(const glm::vec4& color) { modulate_ = color; QueueRedraw(); }
    glm::vec4 GetModulate() const { return modulate_; }
    
    // Flip
    void SetFlipH(bool flip) { flipH_ = flip; QueueRedraw(); }
    bool GetFlipH() const { return flipH_; }
    void SetFlipV(bool flip) { flipV_ = flip; QueueRedraw(); }
    bool GetFlipV() const { return flipV_; }
    
    // Override
    glm::vec2 GetMinimumSize() const override;
    void Draw() override;
    
private:
    glm::vec4 CalculateDrawRect() const;
    
private:
    uint32_t textureId_ = 0;
    glm::vec2 textureSize_{0.0f, 0.0f};
    glm::vec4 uvRect_{0.0f, 0.0f, 1.0f, 1.0f};  // u0, v0, u1, v1
    
    StretchMode stretchMode_ = StretchMode::SCALE;
    glm::vec4 modulate_{1.0f, 1.0f, 1.0f, 1.0f};
    
    bool flipH_ = false;
    bool flipV_ = false;
};

/**
 * @class UIColorRect
 * @brief Widget that displays a solid color rectangle.
 *
 * Features:
 * - Simple solid color fill
 * - Good for backgrounds and separators
 */
class UIColorRect : public UIControl {
public:
    UIColorRect();
    explicit UIColorRect(const glm::vec4& color);
    ~UIColorRect() override = default;
    
    // Color
    void SetColor(const glm::vec4& color) { color_ = color; QueueRedraw(); }
    glm::vec4 GetColor() const { return color_; }
    
    // Override
    void Draw() override;
    
private:
    glm::vec4 color_{1.0f, 1.0f, 1.0f, 1.0f};
};

}  // namespace se::ui
