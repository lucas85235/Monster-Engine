#pragma once

#include <se_pch.h>
#include <cstdint>

namespace se::ui {

/**
 * @struct UIGlyph
 * @brief Data for a single cached glyph
 */
struct UIGlyph {
    // UV coordinates in texture atlas (0-1 range)
    float u0 = 0.0f, v0 = 0.0f;
    float u1 = 0.0f, v1 = 0.0f;
    
    // Glyph metrics in pixels
    int width = 0;
    int height = 0;
    int bearingX = 0;  // Offset from origin to left edge
    int bearingY = 0;  // Offset from baseline to top edge
    int advance = 0;   // Horizontal advance to next glyph
    
    // Position in atlas (pixels)
    int atlasX = 0;
    int atlasY = 0;
    
    bool valid = false;
};

}  // namespace se::ui
