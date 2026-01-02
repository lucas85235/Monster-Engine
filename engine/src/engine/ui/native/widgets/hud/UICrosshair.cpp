#include "engine/ui/native/widgets/hud/UICrosshair.h"
#include "engine/ui/native/render/UICanvas2D.h"

namespace se::ui {

UICrosshair::UICrosshair() {
    SetName("UICrosshair");
    SetMouseFilter(MouseFilter::MOUSE_IGNORE);
}

glm::vec2 UICrosshair::GetMinimumSize() const {
    float totalSize = (lineLength_ + gap_) * 2.0f;
    return glm::max(GetCustomMinimumSize(), glm::vec2{totalSize, totalSize});
}

void UICrosshair::Draw() {
    auto& canvas = UICanvas2D::Get();
    glm::vec2 pos = GetGlobalPosition();
    glm::vec2 size = GetSize();

    float centerX = pos.x + size.x * 0.5f;
    float centerY = pos.y + size.y * 0.5f;

    float halfThickness = lineThickness_ * 0.5f;
    float outlineOffset = outlineThickness_;

    // Draw outline first (if enabled)
    if (outlineEnabled_) {
        float outlineHalfThickness = halfThickness + outlineOffset;

        // Top arm outline
        canvas.DrawRectFilled(
            {centerX - outlineHalfThickness, centerY - gap_ - lineLength_ - outlineOffset,
             lineThickness_ + outlineOffset * 2.0f, lineLength_ + outlineOffset * 2.0f},
            outlineColor_);

        // Bottom arm outline
        canvas.DrawRectFilled(
            {centerX - outlineHalfThickness, centerY + gap_ - outlineOffset,
             lineThickness_ + outlineOffset * 2.0f, lineLength_ + outlineOffset * 2.0f},
            outlineColor_);

        // Left arm outline
        canvas.DrawRectFilled(
            {centerX - gap_ - lineLength_ - outlineOffset, centerY - outlineHalfThickness,
             lineLength_ + outlineOffset * 2.0f, lineThickness_ + outlineOffset * 2.0f},
            outlineColor_);

        // Right arm outline
        canvas.DrawRectFilled(
            {centerX + gap_ - outlineOffset, centerY - outlineHalfThickness,
             lineLength_ + outlineOffset * 2.0f, lineThickness_ + outlineOffset * 2.0f},
            outlineColor_);
    }

    // Draw main crosshair lines
    // Top arm
    canvas.DrawRectFilled(
        {centerX - halfThickness, centerY - gap_ - lineLength_,
         lineThickness_, lineLength_},
        color_);

    // Bottom arm
    canvas.DrawRectFilled(
        {centerX - halfThickness, centerY + gap_,
         lineThickness_, lineLength_},
        color_);

    // Left arm
    canvas.DrawRectFilled(
        {centerX - gap_ - lineLength_, centerY - halfThickness,
         lineLength_, lineThickness_},
        color_);

    // Right arm
    canvas.DrawRectFilled(
        {centerX + gap_, centerY - halfThickness,
         lineLength_, lineThickness_},
        color_);
}

}  // namespace se::ui
