#pragma once
/**
 * InputHandler.h - Keyboard shortcut and input processing.
 *
 * Centralizes all keyboard shortcut handling, separating input logic
 * from the main layer. Uses action bindings for flexibility.
 */

#include <functional>
#include <unordered_map>
#include <string>

#include "Engine.h"

namespace mst {

class EditorContext;

class InputHandler {
public:
    using ActionCallback = std::function<void()>;
    
    void ProcessShortcuts(EditorContext& ctx, bool viewportHovered, bool viewportFocused);
    
    void SetEnabled(bool enabled) { enabled_ = enabled; }
    bool IsEnabled() const { return enabled_; }

private:
    void ProcessGizmoShortcuts(EditorContext& ctx);
    void ProcessEntityShortcuts(EditorContext& ctx);
    void ProcessViewShortcuts(EditorContext& ctx);
    void ProcessFileShortcuts(EditorContext& ctx);
    
    bool IsKeyJustPressed(int key);
    bool IsKeyDown(int key) const;
    bool IsCtrlDown() const;
    
    bool enabled_ = true;
    bool viewportActive_ = false;
    
    std::unordered_map<int, bool> previousKeyStates_;
};

}  // namespace mst
