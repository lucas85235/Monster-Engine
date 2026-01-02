#pragma once

#include <cstdint>

namespace se::ui {

// Side indices for anchors/offsets arrays
enum Side : uint8_t {
    SIDE_LEFT = 0,
    SIDE_TOP = 1,
    SIDE_RIGHT = 2,
    SIDE_BOTTOM = 3,
    SIDE_MAX = 4
};

// Layout mode determines how position/size is calculated
enum class LayoutMode : uint8_t {
    POSITION,       // Direct position/size values
    ANCHORS,        // Anchor-based responsive layout
    CONTAINER,      // Managed by parent container
    UNCONTROLLED    // External control (e.g., script)
};

// Preset anchor configurations for quick setup
enum class LayoutPreset : uint8_t {
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    CENTER_LEFT,
    CENTER_TOP,
    CENTER_RIGHT,
    CENTER_BOTTOM,
    CENTER,
    LEFT_WIDE,          // Full height, left edge
    TOP_WIDE,           // Full width, top edge
    RIGHT_WIDE,         // Full height, right edge
    BOTTOM_WIDE,        // Full width, bottom edge
    VCENTER_WIDE,       // Full height, centered horizontally
    HCENTER_WIDE,       // Full width, centered vertically
    FULL_RECT,          // Fill entire parent
    PRESET_MAX
};

// Size flags for container layout
enum SizeFlags : uint8_t {
    SIZE_SHRINK_BEGIN   = 0,        // Align to start of available space
    SIZE_FILL           = 1 << 0,   // Fill available space
    SIZE_EXPAND         = 1 << 1,   // Request extra space from container
    SIZE_SHRINK_CENTER  = 1 << 2,   // Align to center of available space
    SIZE_SHRINK_END     = 1 << 3,   // Align to end of available space
    SIZE_EXPAND_FILL    = SIZE_EXPAND | SIZE_FILL
};

// Grow direction when minimum size forces growth
enum class GrowDirection : uint8_t {
    BEGIN,      // Grow from start (left/top)
    END,        // Grow from end (right/bottom)
    BOTH        // Grow in both directions
};

// Mouse event handling mode
enum class MouseFilter : uint8_t {
    MOUSE_STOP,     // Consume mouse events, stop propagation
    MOUSE_PASS,     // Receive events but propagate to parent
    MOUSE_IGNORE    // Transparent to mouse (pass through)
};

// Focus acquisition mode
enum class FocusMode : uint8_t {
    NO_FOCUS,       // Cannot receive focus
    CLICK,          // Focus only via mouse click
    ALL_FOCUS,      // Focus via click, Tab, or arrow keys
    ACCESSIBILITY   // Special accessibility mode
};

// Cursor shapes
enum class CursorShape : uint8_t {
    ARROW,
    IBEAM,
    POINTING_HAND,
    CROSSHAIR,
    WAIT_CURSOR,
    BUSY,
    DRAG,
    CAN_DROP,
    FORBIDDEN,
    VSIZE,
    HSIZE,
    BDIAGSIZE,
    FDIAGSIZE,
    MOVE,
    VSPLIT,
    HSPLIT,
    CURSOR_HELP,
    CURSOR_MAX
};

// Control notifications
enum class ControlNotification : uint16_t {
    // CanvasItem notifications
    TRANSFORM_CHANGED = 2000,
    NOTIFICATION_DRAW = 30,
    VISIBILITY_CHANGED = 31,
    ENTER_CANVAS = 32,
    EXIT_CANVAS = 33,
    LOCAL_TRANSFORM_CHANGED = 35,
    
    // Control notifications
    RESIZED = 40,
    MOUSE_ENTER = 41,
    MOUSE_EXIT = 42,
    FOCUS_ENTER = 43,
    FOCUS_EXIT = 44,
    THEME_CHANGED = 45,
    SCROLL_BEGIN = 47,
    SCROLL_END = 48,
    LAYOUT_DIRECTION_CHANGED = 49,
    
    // Container notifications
    PRE_SORT_CHILDREN = 50,
    SORT_CHILDREN = 51
};

}  // namespace se::ui
