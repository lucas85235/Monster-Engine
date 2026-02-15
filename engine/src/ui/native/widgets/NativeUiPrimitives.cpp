#include "engine/ui/native/widgets/NativeUiPrimitives.h"

namespace se::ui::widgets {

void DrawPanel(NativeUiRenderer& renderer, const Rect& rect, const PanelStyle& style) {
    if (rect.w <= 0.0f || rect.h <= 0.0f) {
        return;
    }

    renderer.DrawFilledRect(rect.x, rect.y, rect.w, rect.h, style.background);
    renderer.DrawRect(rect.x, rect.y, rect.w, rect.h, style.borderThickness, style.border);
}

void DrawButton(NativeUiRenderer& renderer, const Rect& rect, std::string_view label,
                const ButtonStyle& style) {
    DrawPanel(renderer, rect, style.panel);
    renderer.DrawTextAligned(label, rect.x, rect.y, rect.w, rect.h, style.text, style.textScale,
                             style.horizontalAlign, style.verticalAlign);
}

}  // namespace se::ui::widgets
