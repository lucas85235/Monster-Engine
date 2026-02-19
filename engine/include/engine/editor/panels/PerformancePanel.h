#pragma once

#include "engine/editor/EditorPanel.h"

namespace se {

/**
 * Performance panel — displays FPS, frame time, entity count.
 */
class PerformancePanel : public EditorPanel {
   public:
    void OnImGuiRender() override;
    const char* GetName() const override { return "Performance"; }
};

}  // namespace se
