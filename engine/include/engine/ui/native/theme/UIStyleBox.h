#pragma once

#include <memory>

namespace se::ui {

/**
 * @class UIStyleBox
 * @brief Abstract base class for UI backgrounds (equivalent to Godot's StyleBox).
 *
 * StyleBox handles:
 * - Content margins for interior positioning
 * - Drawing the background
 * - Hit testing (for non-rectangular shapes)
 */
class UIStyleBox {
public:
    using Ptr = std::shared_ptr<UIStyleBox>;

    virtual ~UIStyleBox() = default;

    // ─────────────────────────────────────────────────────────
    // Content Margins
    // ─────────────────────────────────────────────────────────

    void SetContentMargin(int side, float margin);
    float GetContentMargin(int side) const;
    
    void SetContentMarginAll(float margin);
    void SetContentMargins(float left, float top, float right, float bottom);
    
    glm::vec4 GetContentMargins() const {
        return {contentMargins_[0], contentMargins_[1], contentMargins_[2], contentMargins_[3]};
    }
    
    /// Get offset to content area from top-left
    glm::vec2 GetContentOffset() const {
        return {contentMargins_[0], contentMargins_[1]};
    }

    // ─────────────────────────────────────────────────────────
    // Size
    // ─────────────────────────────────────────────────────────

    virtual glm::vec2 GetMinimumSize() const;
    
    /// Get the actual draw rect (may be larger than requested for effects)
    virtual glm::vec4 GetDrawRect(const glm::vec4& rect) const { return rect; }

    // ─────────────────────────────────────────────────────────
    // Drawing
    // ─────────────────────────────────────────────────────────

    /// Draw the style box at the given rect (x, y, width, height)
    virtual void Draw(const glm::vec4& rect) const = 0;

    // ─────────────────────────────────────────────────────────
    // Hit Testing
    // ─────────────────────────────────────────────────────────

    /// Test if a point is inside this style (for non-rectangular shapes)
    virtual bool TestMask(const glm::vec2& point, const glm::vec4& rect) const;

protected:
    float contentMargins_[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // LTRB
};

/**
 * @class UIStyleBoxEmpty
 * @brief Empty style box that draws nothing
 */
class UIStyleBoxEmpty : public UIStyleBox {
public:
    void Draw(const glm::vec4& rect) const override;
};

/**
 * @class UIStyleBoxFlat
 * @brief Solid color style box with optional border and rounded corners
 */
class UIStyleBoxFlat : public UIStyleBox {
public:
    UIStyleBoxFlat();
    
    void Draw(const glm::vec4& rect) const override;

    // Background
    void SetBackgroundColor(const glm::vec4& color) { bgColor_ = color; }
    glm::vec4 GetBackgroundColor() const { return bgColor_; }

    // Border
    void SetBorderColor(const glm::vec4& color) { borderColor_ = color; }
    glm::vec4 GetBorderColor() const { return borderColor_; }
    
    void SetBorderWidth(float width) { SetBorderWidthAll(width); }
    void SetBorderWidthAll(float width);
    void SetBorderWidths(float left, float top, float right, float bottom);
    float GetBorderWidth(int side) const { return borderWidths_[side]; }
    
    // Corner radius
    void SetCornerRadius(float radius) { SetCornerRadiusAll(radius); }
    void SetCornerRadiusAll(float radius);
    void SetCornerRadii(float topLeft, float topRight, float bottomRight, float bottomLeft);
    float GetCornerRadius(int corner) const { return cornerRadii_[corner]; }
    
    // Shadow (optional)
    void SetShadowColor(const glm::vec4& color) { shadowColor_ = color; }
    void SetShadowSize(float size) { shadowSize_ = size; }
    void SetShadowOffset(const glm::vec2& offset) { shadowOffset_ = offset; }

    glm::vec2 GetMinimumSize() const override;

private:
    glm::vec4 bgColor_{0.2f, 0.2f, 0.2f, 1.0f};
    glm::vec4 borderColor_{0.5f, 0.5f, 0.5f, 1.0f};
    float borderWidths_[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // LTRB
    float cornerRadii_[4] = {0.0f, 0.0f, 0.0f, 0.0f};   // TL, TR, BR, BL
    
    glm::vec4 shadowColor_{0.0f, 0.0f, 0.0f, 0.0f};
    float shadowSize_ = 0.0f;
    glm::vec2 shadowOffset_{0.0f, 0.0f};
};

/**
 * @class UIStyleBoxTexture
 * @brief 9-patch texture style box
 */
class UIStyleBoxTexture : public UIStyleBox {
public:
    UIStyleBoxTexture();
    
    void Draw(const glm::vec4& rect) const override;

    void SetTexture(uint32_t textureId) { textureId_ = textureId; }
    uint32_t GetTexture() const { return textureId_; }
    
    // 9-patch margins (how much of texture is border)
    void SetMargin(int side, float margin);
    float GetMargin(int side) const { return patchMargins_[side]; }
    void SetMarginAll(float margin);
    
    void SetModulateColor(const glm::vec4& color) { modulateColor_ = color; }
    glm::vec4 GetModulateColor() const { return modulateColor_; }
    
    void SetSourceRect(const glm::vec4& rect) { sourceRect_ = rect; }
    glm::vec4 GetSourceRect() const { return sourceRect_; }
    
    // Axis stretch mode
    enum class AxisStretchMode {
        STRETCH,
        TILE,
        TILE_FIT
    };
    
    void SetAxisStretchHorizontal(AxisStretchMode mode) { hStretchMode_ = mode; }
    void SetAxisStretchVertical(AxisStretchMode mode) { vStretchMode_ = mode; }

    glm::vec2 GetMinimumSize() const override;

private:
    uint32_t textureId_ = 0;
    float patchMargins_[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // LTRB
    glm::vec4 modulateColor_{1.0f, 1.0f, 1.0f, 1.0f};
    glm::vec4 sourceRect_{0.0f, 0.0f, 1.0f, 1.0f};  // UV coordinates
    AxisStretchMode hStretchMode_ = AxisStretchMode::STRETCH;
    AxisStretchMode vStretchMode_ = AxisStretchMode::STRETCH;
};

}  // namespace se::ui
