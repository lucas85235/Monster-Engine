#pragma once

#include <string_view>

#include "engine/ui/native/NativeUiRenderer.h"

namespace se::ui::widgets {

struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    float Right() const {
        return x + w;
    }

    float Bottom() const {
        return y + h;
    }

    bool Contains(float px, float py) const {
        return px >= x && py >= y && px <= Right() && py <= Bottom();
    }
};

struct PanelStyle {
    NativeUiColor background;
    NativeUiColor border;
    float         borderThickness = 1.0f;
};

struct ButtonStyle {
    PanelStyle panel;
    NativeUiColor text;
    float textScale = 1.0f;
    NativeUiRenderer::TextHorizontalAlign horizontalAlign =
        NativeUiRenderer::TextHorizontalAlign::Center;
    NativeUiRenderer::TextVerticalAlign verticalAlign =
        NativeUiRenderer::TextVerticalAlign::Center;
};

void DrawPanel(NativeUiRenderer& renderer, const Rect& rect, const PanelStyle& style);
void DrawButton(NativeUiRenderer& renderer, const Rect& rect, std::string_view label,
                const ButtonStyle& style);

}  // namespace se::ui::widgets
