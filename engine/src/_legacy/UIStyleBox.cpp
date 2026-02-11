#include "engine/ui/native/theme/UIStyleBox.h"
#include "engine/ui/native/render/UICanvas2D.h"
#include "engine/Log.h"

#include <algorithm>

namespace se::ui {

// ─────────────────────────────────────────────────────────
// UIStyleBox Base
// ─────────────────────────────────────────────────────────

void UIStyleBox::SetContentMargin(int side, float margin) {
    if (side < 0 || side >= 4) return;
    contentMargins_[side] = std::max(0.0f, margin);
}

float UIStyleBox::GetContentMargin(int side) const {
    if (side < 0 || side >= 4) return 0.0f;
    return contentMargins_[side];
}

void UIStyleBox::SetContentMarginAll(float margin) {
    margin = std::max(0.0f, margin);
    for (int i = 0; i < 4; ++i) {
        contentMargins_[i] = margin;
    }
}

void UIStyleBox::SetContentMargins(float left, float top, float right, float bottom) {
    contentMargins_[0] = std::max(0.0f, left);
    contentMargins_[1] = std::max(0.0f, top);
    contentMargins_[2] = std::max(0.0f, right);
    contentMargins_[3] = std::max(0.0f, bottom);
}

glm::vec2 UIStyleBox::GetMinimumSize() const {
    return {
        contentMargins_[0] + contentMargins_[2],  // left + right
        contentMargins_[1] + contentMargins_[3]   // top + bottom
    };
}

bool UIStyleBox::TestMask(const glm::vec2& point, const glm::vec4& rect) const {
    // Default: simple rectangle test
    return point.x >= rect.x && point.x <= rect.x + rect.z &&
           point.y >= rect.y && point.y <= rect.y + rect.w;
}

// ─────────────────────────────────────────────────────────
// UIStyleBoxEmpty
// ─────────────────────────────────────────────────────────

void UIStyleBoxEmpty::Draw(const glm::vec4& rect) const {
    // Draw nothing
}

// ─────────────────────────────────────────────────────────
// UIStyleBoxFlat
// ─────────────────────────────────────────────────────────

UIStyleBoxFlat::UIStyleBoxFlat() {
    // Set default content margins equal to border widths
}

void UIStyleBoxFlat::SetBorderWidthAll(float width) {
    width = std::max(0.0f, width);
    for (int i = 0; i < 4; ++i) {
        borderWidths_[i] = width;
    }
}

void UIStyleBoxFlat::SetBorderWidths(float left, float top, float right, float bottom) {
    borderWidths_[0] = std::max(0.0f, left);
    borderWidths_[1] = std::max(0.0f, top);
    borderWidths_[2] = std::max(0.0f, right);
    borderWidths_[3] = std::max(0.0f, bottom);
}

void UIStyleBoxFlat::SetCornerRadiusAll(float radius) {
    radius = std::max(0.0f, radius);
    for (int i = 0; i < 4; ++i) {
        cornerRadii_[i] = radius;
    }
}

void UIStyleBoxFlat::SetCornerRadii(float topLeft, float topRight, float bottomRight, float bottomLeft) {
    cornerRadii_[0] = std::max(0.0f, topLeft);
    cornerRadii_[1] = std::max(0.0f, topRight);
    cornerRadii_[2] = std::max(0.0f, bottomRight);
    cornerRadii_[3] = std::max(0.0f, bottomLeft);
}

glm::vec2 UIStyleBoxFlat::GetMinimumSize() const {
    // Min size is max of content margins and border widths
    float minW = std::max(contentMargins_[0] + contentMargins_[2], 
                          borderWidths_[0] + borderWidths_[2]);
    float minH = std::max(contentMargins_[1] + contentMargins_[3],
                          borderWidths_[1] + borderWidths_[3]);
    
    // Also account for corner radii
    float maxRadius = std::max({cornerRadii_[0], cornerRadii_[1], cornerRadii_[2], cornerRadii_[3]});
    minW = std::max(minW, maxRadius * 2.0f);
    minH = std::max(minH, maxRadius * 2.0f);
    
    return {minW, minH};
}

void UIStyleBoxFlat::Draw(const glm::vec4& rect) const {
    auto& canvas = UICanvas2D::Get();
    
    // Draw shadow if enabled
    if (shadowSize_ > 0.0f) {
        glm::vec4 shadowRect{
            rect.x + shadowOffset_.x,
            rect.y + shadowOffset_.y,
            rect.z + shadowSize_ * 2.0f,
            rect.w + shadowSize_ * 2.0f
        };
        canvas.DrawRectFilled(shadowRect, shadowColor_, cornerRadii_[0]);
    }
    
    // Draw background
    float avgRadius = (cornerRadii_[0] + cornerRadii_[1] + cornerRadii_[2] + cornerRadii_[3]) / 4.0f;
    canvas.DrawRectFilled(rect, bgColor_, avgRadius);
    
    // Draw border if any width > 0
    float maxBorder = std::max({borderWidths_[0], borderWidths_[1], borderWidths_[2], borderWidths_[3]});
    if (maxBorder > 0.0f) {
        canvas.DrawRect(rect, borderColor_, maxBorder);
    }
}

// ─────────────────────────────────────────────────────────
// UIStyleBoxTexture
// ─────────────────────────────────────────────────────────

UIStyleBoxTexture::UIStyleBoxTexture() = default;

void UIStyleBoxTexture::SetMargin(int side, float margin) {
    if (side < 0 || side >= 4) return;
    patchMargins_[side] = std::max(0.0f, margin);
}

void UIStyleBoxTexture::SetMarginAll(float margin) {
    margin = std::max(0.0f, margin);
    for (int i = 0; i < 4; ++i) {
        patchMargins_[i] = margin;
    }
}

glm::vec2 UIStyleBoxTexture::GetMinimumSize() const {
    // 9-patch min size = sum of patch margins
    return {
        patchMargins_[0] + patchMargins_[2],
        patchMargins_[1] + patchMargins_[3]
    };
}

void UIStyleBoxTexture::Draw(const glm::vec4& rect) const {
    if (textureId_ == 0) return;
    
    auto& canvas = UICanvas2D::Get();
    
    // Simple 9-patch implementation
    // For now, just draw the texture stretched (full 9-patch in future iteration)
    canvas.DrawNinePatch(
        textureId_,
        rect,
        sourceRect_,
        glm::vec4{patchMargins_[0], patchMargins_[1], patchMargins_[2], patchMargins_[3]},
        modulateColor_
    );
}

}  // namespace se::ui
