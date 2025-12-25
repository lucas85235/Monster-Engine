#pragma once
/**
 * StatusBarPanel.h - Bottom status bar displaying editor state.
 *
 * Shows entity count, selection info, toggle states, and FPS.
 */

#include "IPanel.h"

namespace mst {

class StatusBarPanel : public IPanel {
public:
    void Render(EditorContext& ctx) override;
    const char* GetName() const override { return "Status Bar"; }
    
    void SetGridVisible(bool visible) { gridVisible_ = visible; }
    void SetColliderDebugVisible(bool visible) { colliderDebugVisible_ = visible; }

private:
    bool gridVisible_ = true;
    bool colliderDebugVisible_ = false;
};

}  // namespace mst
