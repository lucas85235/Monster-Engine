#include "engine/ui/native/UIBoxContainer.h"
#include "engine/Log.h"

#include <algorithm>
#include <unordered_map>

namespace se::ui {

UIBoxContainer::UIBoxContainer(bool vertical) : vertical_(vertical) {
    SE_LOG_DEBUG("UIBoxContainer created ({})", vertical_ ? "vertical" : "horizontal");
}

UIBoxContainer::~UIBoxContainer() = default;

glm::vec2 UIBoxContainer::GetMinimumSize() const {
    glm::vec2 minSize{0.0f, 0.0f};
    int visibleChildren = 0;

    for (const auto& child : GetChildren()) {
        if (!child->IsVisible()) continue;
        
        glm::vec2 childMin = child->GetCombinedMinimumSize();
        
        if (vertical_) {
            minSize.x = std::max(minSize.x, childMin.x);
            minSize.y += childMin.y;
        } else {
            minSize.x += childMin.x;
            minSize.y = std::max(minSize.y, childMin.y);
        }
        
        visibleChildren++;
    }
    
    // Add separation between children
    if (visibleChildren > 1) {
        float totalSeparation = static_cast<float>(separation_ * (visibleChildren - 1));
        if (vertical_) {
            minSize.y += totalSeparation;
        } else {
            minSize.x += totalSeparation;
        }
    }
    
    return minSize;
}

void UIBoxContainer::PerformLayout() {
    const auto& children = GetChildren();
    glm::vec2 containerSize = GetSize();
    
    // Count visible children and build cache
    std::vector<std::pair<UIControl*, MinSizeCache>> childCache;
    childCache.reserve(children.size());
    
    float stretchMin = 0.0f;
    float stretchRatioTotal = 0.0f;
    
    // PASS 1: Calculate minimum sizes and stretch ratios
    for (const auto& child : children) {
        if (!child->IsVisible()) continue;
        
        glm::vec2 childMinSize = child->GetCombinedMinimumSize();
        
        MinSizeCache cache;
        cache.minSize = vertical_ ? childMinSize.y : childMinSize.x;
        
        uint8_t sizeFlags = vertical_ ? child->GetVSizeFlags() : child->GetHSizeFlags();
        cache.willStretch = (sizeFlags & SIZE_EXPAND) != 0;
        
        if (cache.willStretch) {
            stretchRatioTotal += child->GetStretchRatio();
        }
        
        stretchMin += cache.minSize;
        childCache.push_back({child.get(), cache});
    }
    
    if (childCache.empty()) return;
    
    // Calculate available space
    float totalSeparation = static_cast<float>(separation_ * (static_cast<int>(childCache.size()) - 1));
    float availableSpace = (vertical_ ? containerSize.y : containerSize.x) - totalSeparation;
    float stretchSpace = std::max(0.0f, availableSpace - stretchMin);
    
    // PASS 2: Distribute stretch space to expanding elements
    if (stretchRatioTotal > 0.0f && stretchSpace > 0.0f) {
        // Iterative distribution to handle elements that can't shrink enough
        float remainingSpace = stretchSpace;
        float remainingRatio = stretchRatioTotal;
        
        // Simple distribution (could be made iterative for edge cases)
        for (auto& [child, cache] : childCache) {
            if (cache.willStretch && remainingRatio > 0.0f) {
                float ratio = child->GetStretchRatio() / remainingRatio;
                float extraSpace = remainingSpace * ratio;
                cache.finalSize = cache.minSize + extraSpace;
            } else {
                cache.finalSize = cache.minSize;
            }
        }
    } else {
        // No stretching, just use minimum sizes
        for (auto& [child, cache] : childCache) {
            cache.finalSize = cache.minSize;
        }
    }
    
    // PASS 3: Position children
    float offset = 0.0f;
    
    // Handle alignment
    if (alignment_ != 0) {
        float totalUsed = totalSeparation;
        for (const auto& [child, cache] : childCache) {
            totalUsed += cache.finalSize;
        }
        
        float freeSpace = (vertical_ ? containerSize.y : containerSize.x) - totalUsed;
        if (freeSpace > 0.0f) {
            if (alignment_ == 1) {
                offset = freeSpace * 0.5f;  // Center
            } else if (alignment_ == 2) {
                offset = freeSpace;  // End
            }
        }
    }
    
    for (auto& [child, cache] : childCache) {
        glm::vec2 childMinSize = child->GetCombinedMinimumSize();
        glm::vec2 childPos, childSize;
        
        if (vertical_) {
            childPos = {0.0f, offset};
            childSize = {containerSize.x, cache.finalSize};
        } else {
            childPos = {offset, 0.0f};
            childSize = {cache.finalSize, containerSize.y};
        }
        
        FitChildInRect(child, childPos, childSize);
        
        offset += cache.finalSize + static_cast<float>(separation_);
    }
    
    SE_LOG_DEBUG("UIBoxContainer layout: {} children, {} space", 
                 childCache.size(), vertical_ ? "vertical" : "horizontal");
}

}  // namespace se::ui
