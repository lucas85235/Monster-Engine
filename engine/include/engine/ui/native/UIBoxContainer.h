#pragma once

#include "engine/ui/native/UIContainer.h"

namespace se::ui {

/**
 * @class UIBoxContainer
 * @brief Container that arranges children in a single row or column (HBox/VBox).
 *
 * Uses Godot's 3-pass layout algorithm:
 * 1. Calculate minimum sizes and stretch ratios
 * 2. Distribute extra space to expanding elements
 * 3. Position children with size flags
 */
class UIBoxContainer : public UIContainer {
public:
    UIBoxContainer(bool vertical = false);
    ~UIBoxContainer() override;

    void SetVertical(bool vertical) { vertical_ = vertical; UpdateMinimumSize(); QueueSort(); }
    bool IsVertical() const { return vertical_; }

    void SetSeparation(int separation) { separation_ = separation; UpdateMinimumSize(); QueueSort(); }
    int GetSeparation() const { return separation_; }

    void SetAlignment(int alignment) { alignment_ = alignment; QueueSort(); }
    int GetAlignment() const { return alignment_; }

    glm::vec2 GetMinimumSize() const override;

protected:
    void PerformLayout() override;

private:
    // Cache for layout calculation
    struct MinSizeCache {
        float minSize = 0.0f;
        float finalSize = 0.0f;
        bool willStretch = false;
    };

    bool vertical_ = false;
    int separation_ = 4;
    int alignment_ = 0;  // 0 = begin, 1 = center, 2 = end
};

/**
 * @class UIHBoxContainer
 * @brief Horizontal box container (convenience alias)
 */
class UIHBoxContainer : public UIBoxContainer {
public:
    UIHBoxContainer() : UIBoxContainer(false) {}
};

/**
 * @class UIVBoxContainer
 * @brief Vertical box container (convenience alias)
 */
class UIVBoxContainer : public UIBoxContainer {
public:
    UIVBoxContainer() : UIBoxContainer(true) {}
};

}  // namespace se::ui
