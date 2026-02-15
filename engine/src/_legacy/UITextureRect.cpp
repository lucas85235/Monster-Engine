#include "engine/ui/native/widgets/UITextureRect.h"
#include "engine/ui/native/render/UICanvas2D.h"

#include <algorithm>
#include <cmath>

namespace se::ui {

// ============================================================
// UITextureRect
// ============================================================

UITextureRect::UITextureRect() {
    SetName("UITextureRect");
}

glm::vec2 UITextureRect::GetMinimumSize() const {
    glm::vec2 minSize = GetCustomMinimumSize();
    
    // For KEEP modes, minimum size is the texture size
    if (stretchMode_ == StretchMode::KEEP || 
        stretchMode_ == StretchMode::KEEP_CENTERED) {
        if (textureSize_.x > 0.0f && textureSize_.y > 0.0f) {
            minSize.x = std::max(minSize.x, textureSize_.x);
            minSize.y = std::max(minSize.y, textureSize_.y);
        }
    }
    
    return minSize;
}

void UITextureRect::Draw() {
    if (textureId_ == 0) return;
    
    auto& canvas = UICanvas2D::Get();
    glm::vec4 drawRect = CalculateDrawRect();
    
    // Apply UV flipping
    glm::vec4 uv = uvRect_;
    if (flipH_) {
        std::swap(uv.x, uv.z);  // Swap u0 and u1
    }
    if (flipV_) {
        std::swap(uv.y, uv.w);  // Swap v0 and v1
    }
    
    canvas.DrawTexture(textureId_, drawRect, uv, modulate_);
}

glm::vec4 UITextureRect::CalculateDrawRect() const {
    glm::vec2 pos = GetGlobalPosition();
    glm::vec2 size = GetSize();
    
    if (textureSize_.x <= 0.0f || textureSize_.y <= 0.0f) {
        // No texture size info, just fill
        return {pos.x, pos.y, size.x, size.y};
    }
    
    float aspectRatio = textureSize_.x / textureSize_.y;
    float containerAspect = size.x / size.y;
    
    switch (stretchMode_) {
        case StretchMode::SCALE:
            return {pos.x, pos.y, size.x, size.y};
            
        case StretchMode::KEEP:
            return {pos.x, pos.y, textureSize_.x, textureSize_.y};
            
        case StretchMode::KEEP_CENTERED: {
            float x = pos.x + (size.x - textureSize_.x) * 0.5f;
            float y = pos.y + (size.y - textureSize_.y) * 0.5f;
            return {x, y, textureSize_.x, textureSize_.y};
        }
            
        case StretchMode::KEEP_ASPECT: {
            float drawW, drawH;
            if (aspectRatio > containerAspect) {
                // Width-limited
                drawW = size.x;
                drawH = size.x / aspectRatio;
            } else {
                // Height-limited
                drawH = size.y;
                drawW = size.y * aspectRatio;
            }
            return {pos.x, pos.y, drawW, drawH};
        }
            
        case StretchMode::KEEP_ASPECT_COVERED: {
            float drawW, drawH;
            if (aspectRatio > containerAspect) {
                // Height-limited (covers width)
                drawH = size.y;
                drawW = size.y * aspectRatio;
            } else {
                // Width-limited (covers height)
                drawW = size.x;
                drawH = size.x / aspectRatio;
            }
            return {pos.x, pos.y, drawW, drawH};
        }
            
        case StretchMode::KEEP_ASPECT_CENTERED: {
            float drawW, drawH;
            if (aspectRatio > containerAspect) {
                drawW = size.x;
                drawH = size.x / aspectRatio;
            } else {
                drawH = size.y;
                drawW = size.y * aspectRatio;
            }
            float x = pos.x + (size.x - drawW) * 0.5f;
            float y = pos.y + (size.y - drawH) * 0.5f;
            return {x, y, drawW, drawH};
        }
    }
    
    return {pos.x, pos.y, size.x, size.y};
}

// ============================================================
// UIColorRect
// ============================================================

UIColorRect::UIColorRect() {
    SetName("UIColorRect");
}

UIColorRect::UIColorRect(const glm::vec4& color) : color_(color) {
    SetName("UIColorRect");
}

void UIColorRect::Draw() {
    auto& canvas = UICanvas2D::Get();
    glm::vec2 pos = GetGlobalPosition();
    glm::vec2 size = GetSize();
    
    canvas.DrawRectFilled({pos.x, pos.y, size.x, size.y}, color_);
}

}  // namespace se::ui
