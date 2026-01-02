# Godot GUI/UI Architecture - Detailed Analysis

## 1. Overview

Godot's UI system is built on a hierarchical node-based architecture with the following main concepts:
- **CanvasItem** → **Control** → **Container/Widget** hierarchy
- **Anchor-based layout** with offsets for responsive design
- **Theme system** for consistent styling across controls
- **Signal-based event system** for input handling
- **RID-based rendering** through RenderingServer

---

## 2. Class Hierarchy

```
Node
└── CanvasItem                        (Base for all 2D drawable nodes)
    └── Control                       (Base for all UI elements)
        ├── Container                 (Base for layout containers)
        │   ├── BoxContainer (VBox/HBox)
        │   ├── GridContainer
        │   ├── MarginContainer
        │   ├── CenterContainer
        │   ├── FlowContainer
        │   ├── SplitContainer
        │   ├── ScrollContainer
        │   └── PanelContainer
        │
        ├── BaseButton                (Base for interactive buttons)
        │   ├── Button
        │   ├── CheckBox
        │   ├── CheckButton
        │   ├── MenuButton
        │   ├── OptionButton
        │   └── TextureButton
        │
        ├── Range                     (Base for value controls)
        │   ├── ProgressBar
        │   ├── Slider (HSlider/VSlider)
        │   ├── SpinBox
        │   └── ScrollBar
        │
        ├── Label
        ├── LineEdit
        ├── TextEdit
        ├── Panel
        ├── TextureRect
        ├── ColorRect
        └── ...
```

---

## 3. CanvasItem - Base 2D Rendering

**Location**: `scene/main/canvas_item.h/cpp`

### 3.1 Key Properties
```cpp
class CanvasItem : public Node {
private:
    RID canvas_item;                    // RenderingServer canvas item handle
    Color modulate;                     // Multiplicative color tint
    Color self_modulate;                // Non-inheriting modulate
    int z_index;                        // Rendering order
    bool visible;                       // Visibility flag
    bool top_level;                     // Detach from parent transform
    Transform2D global_transform;       // Cached world transform
    ClipChildrenMode clip_children_mode;
    TextureFilter texture_filter;
    TextureRepeat texture_repeat;
    Ref<Material> material;
```

### 3.2 Drawing API
```cpp
// Primitive rendering - all create RenderingServer commands
void draw_line(const Point2 &from, const Point2 &to, const Color &color, real_t width);
void draw_rect(const Rect2 &rect, const Color &color, bool filled);
void draw_circle(const Point2 &pos, real_t radius, const Color &color);
void draw_texture(Texture2D &texture, const Point2 &pos, const Color &modulate);
void draw_texture_rect(Texture2D &texture, const Rect2 &rect, bool tile);
void draw_style_box(StyleBox &style_box, const Rect2 &rect);  // Theme integration
void draw_string(Font &font, const Point2 &pos, const String &text, ...);
void draw_polygon(const Vector<Point2> &points, const Vector<Color> &colors);
```

### 3.3 Notifications
```cpp
enum {
    NOTIFICATION_TRANSFORM_CHANGED = 2000,
    NOTIFICATION_DRAW = 30,
    NOTIFICATION_VISIBILITY_CHANGED = 31,
    NOTIFICATION_ENTER_CANVAS = 32,
    NOTIFICATION_EXIT_CANVAS = 33,
    NOTIFICATION_LOCAL_TRANSFORM_CHANGED = 35,
};
```

### 3.4 Transform Propagation
- Global transform is cached and invalidated when parent changes
- `_notify_transform()` propagates changes to children
- Uses a "dirty flag" pattern with `global_invalid`

---

## 4. Control - UI Base Class

**Location**: `scene/gui/control.h/cpp`

### 4.1 Core Data Structure
```cpp
struct Data {
    // Global relations
    Control *parent_control = nullptr;
    Window *parent_window = nullptr;
    CanvasItem *parent_canvas_item = nullptr;

    // Positioning and sizing
    LayoutMode stored_layout_mode = LAYOUT_MODE_POSITION;
    real_t offset[4] = {0, 0, 0, 0};  // Left, Top, Right, Bottom offsets
    real_t anchor[4] = {0, 0, 0, 0};  // Left, Top, Right, Bottom anchors (0.0-1.0)
    GrowDirection h_grow, v_grow;
    real_t rotation = 0.0;
    Vector2 scale = Vector2(1, 1);
    Vector2 pivot_offset;

    Point2 pos_cache;                   // Cached position
    Size2 size_cache;                   // Cached size
    Size2 minimum_size_cache;           // Cached minimum size
    Point2 custom_minimum_size;         // User-defined minimum

    // Container sizing
    BitField<SizeFlags> h_size_flags = SIZE_FILL;
    BitField<SizeFlags> v_size_flags = SIZE_FILL;
    real_t expand = 1.0;                // Stretch ratio for containers

    // Input events
    MouseFilter mouse_filter = MOUSE_FILTER_STOP;
    CursorShape default_cursor = CURSOR_ARROW;

    // Focus
    FocusMode focus_mode = FOCUS_NONE;
    NodePath focus_neighbor[4];         // Up, Down, Left, Right

    // Theming
    ThemeOwner *theme_owner = nullptr;
    Ref<Theme> theme;
    StringName theme_type_variation;
    HashMap<StringName, Ref<Texture2D>> theme_icon_override;
    HashMap<StringName, Ref<StyleBox>> theme_style_override;
    HashMap<StringName, Ref<Font>> theme_font_override;
    HashMap<StringName, int> theme_font_size_override;
    HashMap<StringName, Color> theme_color_override;
    HashMap<StringName, int> theme_constant_override;

    // Internationalization
    LayoutDirection layout_dir = LAYOUT_DIRECTION_INHERITED;
    bool is_rtl;

    // Extra
    String tooltip;
};
```

### 4.2 Anchor/Offset Layout System

**Concept**: Position is calculated relative to parent edges using anchors (0.0-1.0) plus pixel offsets.

```cpp
// Position calculation formula:
// For horizontal: position_x = parent_width * anchor_left + offset_left
// For vertical: position_y = parent_height * anchor_top + offset_top
// Width: width = (parent_width * anchor_right + offset_right) - (parent_width * anchor_left + offset_left)

void Control::_size_changed() {
    Rect2 parent_rect = get_parent_anchorable_rect();
    
    // Calculate new position and size from anchors and offsets
    real_t new_pos_left = parent_rect.size.width * data.anchor[SIDE_LEFT] + data.offset[SIDE_LEFT];
    real_t new_pos_top = parent_rect.size.height * data.anchor[SIDE_TOP] + data.offset[SIDE_TOP];
    real_t new_pos_right = parent_rect.size.width * data.anchor[SIDE_RIGHT] + data.offset[SIDE_RIGHT];
    real_t new_pos_bottom = parent_rect.size.height * data.anchor[SIDE_BOTTOM] + data.offset[SIDE_BOTTOM];

    // Apply grow direction
    Point2 new_pos(new_pos_left, new_pos_top);
    Size2 new_size(new_pos_right - new_pos_left, new_pos_bottom - new_pos_top);

    // Clamp to minimum size
    Size2 minimum = get_combined_minimum_size();
    if (new_size.width < minimum.width) {
        // Adjust based on grow direction...
    }
    // ...
}
```

### 4.3 Layout Modes
```cpp
enum LayoutMode {
    LAYOUT_MODE_POSITION,   // Simple position/size
    LAYOUT_MODE_ANCHORS,    // Anchor-based responsive
    LAYOUT_MODE_CONTAINER,  // Managed by parent container
    LAYOUT_MODE_UNCONTROLLED,
};
```

### 4.4 Layout Presets
```cpp
enum LayoutPreset {
    PRESET_TOP_LEFT,      // Anchors: (0,0,0,0)
    PRESET_TOP_RIGHT,     // Anchors: (1,0,1,0)
    PRESET_BOTTOM_LEFT,   // Anchors: (0,1,0,1)
    PRESET_BOTTOM_RIGHT,  // Anchors: (1,1,1,1)
    PRESET_CENTER,        // Anchors: (0.5,0.5,0.5,0.5)
    PRESET_FULL_RECT,     // Anchors: (0,0,1,1) - stretches to fill
    // ...
};
```

### 4.5 Size Flags (for Container layout)
```cpp
enum SizeFlags {
    SIZE_SHRINK_BEGIN = 0,  // Align to start
    SIZE_FILL = 1,          // Fill available space
    SIZE_EXPAND = 2,        // Request extra space
    SIZE_SHRINK_CENTER = 4, // Align to center
    SIZE_SHRINK_END = 8,    // Align to end
    SIZE_EXPAND_FILL = SIZE_EXPAND | SIZE_FILL,
};
```

### 4.6 Mouse Filter
```cpp
enum MouseFilter {
    MOUSE_FILTER_STOP,    // Consumes mouse events
    MOUSE_FILTER_PASS,    // Receives events but passes to parent
    MOUSE_FILTER_IGNORE,  // Transparent to mouse
};
```

### 4.7 Focus System
```cpp
enum FocusMode {
    FOCUS_NONE,        // Cannot receive keyboard focus
    FOCUS_CLICK,       // Focus only via mouse click
    FOCUS_ALL,         // Focus via click, Tab, or arrow keys
    FOCUS_ACCESSIBILITY,
};

// Focus navigation
Control *find_next_valid_focus() const;
Control *find_prev_valid_focus() const;
void grab_focus();
void release_focus();
```

### 4.8 Control Notifications
```cpp
enum {
    NOTIFICATION_RESIZED = 40,
    NOTIFICATION_MOUSE_ENTER = 41,
    NOTIFICATION_MOUSE_EXIT = 42,
    NOTIFICATION_FOCUS_ENTER = 43,
    NOTIFICATION_FOCUS_EXIT = 44,
    NOTIFICATION_THEME_CHANGED = 45,
    NOTIFICATION_SCROLL_BEGIN = 47,
    NOTIFICATION_SCROLL_END = 48,
    NOTIFICATION_LAYOUT_DIRECTION_CHANGED = 49,
};
```

---

## 5. Container - Layout Management

**Location**: `scene/gui/container.h/cpp`

### 5.1 Base Container
```cpp
class Container : public Control {
    bool pending_sort = false;
    void _sort_children();
    void _child_minsize_changed();

protected:
    void queue_sort();  // Deferred layout update
    Control *as_sortable_control(Node *p_node, SortableVisibilityMode mode);

    void add_child_notify(Node *p_child) override;
    void move_child_notify(Node *p_child) override;
    void remove_child_notify(Node *p_child) override;

public:
    enum {
        NOTIFICATION_PRE_SORT_CHILDREN = 50,
        NOTIFICATION_SORT_CHILDREN = 51,
    };

    void fit_child_in_rect(Control *child, const Rect2 &rect);
};
```

### 5.2 Child Notification Connections
```cpp
void Container::add_child_notify(Node *p_child) {
    Control *control = Object::cast_to<Control>(p_child);
    if (!control) return;

    // Connect to child signals for automatic relayout
    control->connect("size_flags_changed", callable_mp(this, &Container::queue_sort));
    control->connect("minimum_size_changed", callable_mp(this, &Container::_child_minsize_changed));
    control->connect("visibility_changed", callable_mp(this, &Container::_child_minsize_changed));

    update_minimum_size();
    queue_sort();
}
```

### 5.3 fit_child_in_rect (Key Layout Method)
```cpp
void Container::fit_child_in_rect(Control *p_child, const Rect2 &p_rect) {
    Size2 minsize = p_child->get_combined_minimum_size();
    Rect2 r = p_rect;

    // Horizontal sizing based on flags
    if (!(p_child->get_h_size_flags().has_flag(SIZE_FILL))) {
        r.size.x = minsize.width;
        if (p_child->get_h_size_flags().has_flag(SIZE_SHRINK_END)) {
            r.position.x += p_rect.size.width - minsize.width;
        } else if (p_child->get_h_size_flags().has_flag(SIZE_SHRINK_CENTER)) {
            r.position.x += (p_rect.size.x - minsize.width) / 2;
        }
    }

    // Same for vertical...
    p_child->set_rect(r);
    p_child->set_rotation(0);
    p_child->set_scale(Vector2(1, 1));
}
```

### 5.4 BoxContainer Layout Algorithm
```cpp
void BoxContainer::_resort() {
    // PASS 1: Calculate minimum sizes and stretch ratios
    int stretch_min = 0;
    int stretch_avail = 0;
    float stretch_ratio_total = 0.0;
    HashMap<Control *, _MinSizeCache> min_size_cache;

    for (int i = 0; i < get_child_count(); i++) {
        Control *c = as_sortable_control(get_child(i));
        Size2i size = c->get_combined_minimum_size();

        _MinSizeCache msc;
        msc.min_size = vertical ? size.height : size.width;
        msc.will_stretch = vertical 
            ? c->get_v_size_flags().has_flag(SIZE_EXPAND)
            : c->get_h_size_flags().has_flag(SIZE_EXPAND);

        if (msc.will_stretch) {
            stretch_avail += msc.min_size;
            stretch_ratio_total += c->get_stretch_ratio();
        }
        min_size_cache[c] = msc;
    }

    // Calculate available stretch space
    int stretch_max = (vertical ? new_size.height : new_size.width) 
                      - (children_count - 1) * theme_cache.separation;
    int stretch_diff = stretch_max - stretch_min;

    // PASS 2: Distribute extra space to expanding elements
    while (stretch_ratio_total > 0) {
        for (auto &[c, msc] : min_size_cache) {
            if (msc.will_stretch) {
                float final_size = stretch_avail * c->get_stretch_ratio() / stretch_ratio_total;
                if (final_size < msc.min_size) {
                    // Element can't stretch, remove from calculation
                    msc.will_stretch = false;
                    stretch_ratio_total -= c->get_stretch_ratio();
                    stretch_avail -= msc.min_size;
                    break;  // Restart iteration
                }
                msc.final_size = final_size;
            }
        }
    }

    // PASS 3: Position children
    int ofs = 0;
    for (int i = 0; i < get_child_count(); i++) {
        Control *c = as_sortable_control(get_child(i));
        _MinSizeCache &msc = min_size_cache[c];

        Rect2 rect;
        if (vertical) {
            rect = Rect2(0, ofs, new_size.width, msc.final_size);
        } else {
            rect = Rect2(ofs, 0, msc.final_size, new_size.height);
        }

        fit_child_in_rect(c, rect);
        ofs += msc.final_size + theme_cache.separation;
    }
}
```

---

## 6. Theme System

**Location**: `scene/resources/theme.h/cpp`, `scene/theme/theme_db.h/cpp`

### 6.1 Theme Resource
```cpp
class Theme : public Resource {
public:
    // Type aliases for theme item maps
    using ThemeIconMap = HashMap<StringName, Ref<Texture2D>>;
    using ThemeStyleMap = HashMap<StringName, Ref<StyleBox>>;
    using ThemeFontMap = HashMap<StringName, Ref<Font>>;
    using ThemeFontSizeMap = HashMap<StringName, int>;
    using ThemeColorMap = HashMap<StringName, Color>;
    using ThemeConstantMap = HashMap<StringName, int>;

    enum DataType {
        DATA_TYPE_COLOR,
        DATA_TYPE_CONSTANT,
        DATA_TYPE_FONT,
        DATA_TYPE_FONT_SIZE,
        DATA_TYPE_ICON,
        DATA_TYPE_STYLEBOX,
    };

protected:
    // Default values
    float default_base_scale = 0.0;
    Ref<Font> default_font;
    int default_font_size = -1;

    // Per-type item maps: [theme_type][item_name] = value
    HashMap<StringName, ThemeIconMap> icon_map;
    HashMap<StringName, ThemeStyleMap> style_map;
    HashMap<StringName, ThemeFontMap> font_map;
    HashMap<StringName, ThemeFontSizeMap> font_size_map;
    HashMap<StringName, ThemeColorMap> color_map;
    HashMap<StringName, ThemeConstantMap> constant_map;

    // Type variations (e.g., "ButtonRed" inherits from "Button")
    HashMap<StringName, StringName> variation_map;
    HashMap<StringName, List<StringName>> variation_base_map;
};
```

### 6.2 Theme Lookup in Control
```cpp
// Priority order:
// 1. Local theme override
// 2. Theme assigned to this control
// 3. Parent's theme (walking up hierarchy)
// 4. Project theme
// 5. Default theme
// 6. Fallback values

Ref<StyleBox> Control::get_theme_stylebox(const StringName &p_name, const StringName &p_theme_type) {
    StringName type = p_theme_type.is_empty() ? get_theme_type_variation() : p_theme_type;
    if (type.is_empty()) type = get_class_name();

    // Check local override first
    if (data.theme_style_override.has(p_name)) {
        return data.theme_style_override[p_name];
    }

    // Search through theme owner chain
    return data.theme_owner->get_theme_stylebox(p_name, type);
}
```

### 6.3 ThemeDB Singleton
```cpp
class ThemeDB : public Object {
    static ThemeDB *singleton;

    Ref<Theme> default_theme;
    Ref<Theme> project_theme;

    // Fallback values (final fallback)
    float fallback_base_scale = 1.0;
    Ref<Font> fallback_font;
    int fallback_font_size = 16;
    Ref<Texture2D> fallback_icon;
    Ref<StyleBox> fallback_stylebox;

    // Theme item bindings for automatic cache updates
    HashMap<StringName, HashMap<StringName, ThemeItemBind>> theme_item_binds;
};
```

### 6.4 Theme Cache Binding
```cpp
// In widget's _bind_methods():
BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, BoxContainer, separation);
BIND_THEME_ITEM(Theme::DATA_TYPE_STYLEBOX, Button, normal);
BIND_THEME_ITEM(Theme::DATA_TYPE_STYLEBOX, Button, pressed);
BIND_THEME_ITEM(Theme::DATA_TYPE_STYLEBOX, Button, hover);

// This creates a local theme_cache struct in the control:
struct ThemeCache {
    int separation = 0;
    Ref<StyleBox> normal;
    Ref<StyleBox> pressed;
    Ref<StyleBox> hover;
} theme_cache;
```

### 6.5 StyleBox
```cpp
class StyleBox : public Resource {
    float content_margin[4];  // Left, Top, Right, Bottom

public:
    virtual Size2 get_minimum_size() const;
    float get_margin(Side p_side) const;
    Point2 get_offset() const;

    virtual void draw(RID p_canvas_item, const Rect2 &p_rect) const;
    virtual Rect2 get_draw_rect(const Rect2 &p_rect) const;
    virtual bool test_mask(const Point2 &p_point, const Rect2 &p_rect) const;
};

// Implementations:
// - StyleBoxEmpty: No rendering
// - StyleBoxFlat: Solid color with rounded corners, borders
// - StyleBoxTexture: 9-patch texture
// - StyleBoxLine: Single line border
```

---

## 7. Input Handling

### 7.1 Event Flow
```
Window::_input() 
  → Viewport::_gui_input_event()
    → Find control under cursor (hit testing)
    → Sort by canvas layer and Z-order
    → Call Control::_gui_input() on topmost control
      → If accepted: stop propagation
      → If not: pass to parent
```

### 7.2 Control Input Methods
```cpp
class Control {
    // Main input handler - override this
    virtual void gui_input(const Ref<InputEvent> &p_event);
    
    // Stop event propagation
    void accept_event();

    // Hit testing
    virtual bool has_point(const Point2 &p_point) const;
};
```

### 7.3 BaseButton Input Handling
```cpp
void BaseButton::gui_input(const Ref<InputEvent> &p_event) {
    if (status.disabled) return;

    Ref<InputEventMouseButton> mouse_button = p_event;
    bool ui_accept = p_event->is_action("ui_accept", true) && !p_event->is_echo();

    bool button_masked = mouse_button.is_valid() 
        && button_mask.has_flag(mouse_button_to_mask(mouse_button->get_button_index()));

    if (button_masked || ui_accept) {
        on_action_event(p_event);
        return;
    }

    // Track hover state during press attempt
    Ref<InputEventMouseMotion> mouse_motion = p_event;
    if (mouse_motion.is_valid() && status.press_attempt) {
        status.pressing_inside = has_point(mouse_motion->get_position());
        queue_redraw();
    }
}
```

### 7.4 DrawMode States
```cpp
enum DrawMode {
    DRAW_NORMAL,        // Default state
    DRAW_PRESSED,       // Currently pressed
    DRAW_HOVER,         // Mouse hovering
    DRAW_DISABLED,      // Button disabled
    DRAW_HOVER_PRESSED, // Hovering while toggled
};
```

---

## 8. Rendering Integration

### 8.1 Immediate Mode Drawing
Controls override `_draw()` virtual and use `CanvasItem` draw methods:
```cpp
void Button::_draw() {
    DrawMode mode = get_draw_mode();

    // Draw background stylebox
    Ref<StyleBox> style = theme_cache.styles[mode];
    draw_style_box(style, Rect2(Point2(), get_size()));

    // Draw icon
    if (icon.is_valid()) {
        draw_texture(icon, icon_pos, icon_modulate);
    }

    // Draw text
    draw_string(theme_cache.font, text_pos, text, 
                HORIZONTAL_ALIGNMENT_CENTER, -1, 
                theme_cache.font_size, font_color);
}

void Control::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_DRAW:
            // Called when queue_redraw() triggers
            GDVIRTUAL_CALL(_draw);
            break;
    }
}
```

### 8.2 Canvas Item Transform
```cpp
Transform2D Control::_get_internal_transform() const {
    // T(pivot) * R(rotation) * S(scale) * T(-pivot)
    Transform2D xform(data.rotation, data.scale, 0.0f, get_combined_pivot_offset());
    xform.translate_local(-get_combined_pivot_offset());
    return xform;
}

void Control::_update_canvas_item_transform() {
    Transform2D xform = _get_internal_transform();
    xform[2] += get_position();

    // Snap to pixels if enabled
    if (is_inside_tree() && get_viewport()->is_snap_controls_to_pixels_enabled()) {
        xform[2] = (xform[2] + Vector2(0.5, 0.5)).floor();
    }

    RenderingServer::get_singleton()->canvas_item_set_transform(get_canvas_item(), xform);
}
```

---

## 9. Implementation Recommendations for Your Engine

### 9.1 Core Architecture
1. **Widget Base Class** (equivalent to Control)
   - Position/size in local coordinates
   - Parent reference for layout propagation
   - Minimum size calculation (virtual)
   - Theme property access
   - Input event handling (virtual)
   - Draw method (virtual)

2. **Layout Container Base** (equivalent to Container)
   - Child management with layout notifications
   - Deferred layout updates (queue mechanism)
   - `fit_child_in_rect()` helper

3. **Theme System**
   - Resource class holding style definitions
   - Hierarchical lookup (local → parent → default)
   - StyleBox abstraction for backgrounds
   - Color/constant/font specifications

### 9.2 Positioning System Options
**Option A: Anchor-Based (like Godot)**
- Pros: Flexible responsive design
- Cons: More complex to implement and understand

**Option B: Flexbox-Like**
- Pros: Familiar to web developers
- Cons: Less control over absolute positioning

**Option C: Constraint-Based**
- Pros: Powerful for complex layouts
- Cons: Performance concerns with many elements

### 9.3 Rendering Strategy
1. **Immediate Mode** (like Godot)
   - Controls call draw methods in `_draw()` override
   - Commands batched by CanvasItem/RenderingServer
   - Simple but requires careful batching

2. **Retained Mode**
   - Build display list on change
   - Better for static UIs
   - More complex state management

### 9.4 Key Data Structures
```cpp
// Suggested minimal Control implementation
class UIElement {
public:
    // Transform
    Vec2 position;
    Vec2 size;
    Vec2 minSize;  // Cached minimum size
    float rotation = 0;
    Vec2 scale = {1, 1};
    Vec2 pivot;

    // Hierarchy
    UIElement* parent = nullptr;
    std::vector<UIElement*> children;

    // Layout
    SizeFlags hSizeFlags = SIZE_FILL;
    SizeFlags vSizeFlags = SIZE_FILL;
    float stretchRatio = 1.0f;

    // Input
    MouseFilter mouseFilter = MOUSE_FILTER_STOP;
    FocusMode focusMode = FOCUS_NONE;
    bool hasFocus = false;

    // Visibility
    bool visible = true;
    bool clipContents = false;

    // Theme
    Theme* theme = nullptr;
    std::unordered_map<std::string, StyleOverride> styleOverrides;

    // Virtual methods
    virtual Vec2 GetMinimumSize() const;
    virtual void Draw();
    virtual void OnInput(const InputEvent& event);
    virtual void OnNotification(int what);

protected:
    void UpdateMinimumSize();
    void QueueRedraw();
};
```

### 9.5 Implementation Order
1. **Phase 1**: Basic UIElement with position/size, hierarchy, simple rendering
2. **Phase 2**: Minimum size calculation and propagation
3. **Phase 3**: BoxContainer layout (HBox/VBox)
4. **Phase 4**: Theme system (colors, constants)
5. **Phase 5**: StyleBox rendering
6. **Phase 6**: Input handling and focus
7. **Phase 7**: ScrollContainer with clipping
8. **Phase 8**: Advanced widgets (buttons, sliders, text)


---

## 10. Retained Mode Rendering (RendererCanvasCull)

**Location**: `servers/rendering/renderer_canvas_cull.h/cpp`

### 10.1 Architecture Overview
Godot uses a **command-based retained mode** system for 2D rendering. Instead of immediate mode calls, each CanvasItem stores draw commands that are processed in batches during rendering.

### 10.2 Item Structure (Retained Data)
```cpp
struct Item : public RendererCanvasRender::Item {
    RID parent;              // Canvas it belongs to
    RID self;                // Self RID reference
    int z_index;             // Z ordering for rendering
    bool z_relative;         // Relative to parent Z
    bool sort_y;             // Enable Y-sorting
    Color modulate;          // Color modulation
    Color self_modulate;     // Non-inherited modulation
    bool use_parent_material;
    int index;               // Child order index
    bool children_order_dirty;
    
    // Y-sorting data
    int ysort_children_count;
    Transform2D ysort_xform; // Transform for Y-sort position
    int ysort_index;
    int ysort_parent_abs_z_index;
    
    uint32_t visibility_layer = 0xffffffff;
    Vector<Item *> child_items;
    
    // Visibility notifications
    VisibilityNotifierData *visibility_notifier = nullptr;
    DependencyTracker dependency_tracker;
    InstanceUniforms instance_uniforms;
};
```

### 10.3 Canvas Structure
```cpp
struct Canvas : public RendererViewport::CanvasBase {
    HashSet<RID> viewports;          // Viewports showing this canvas
    Vector<ChildItem> child_items;   // Ordered canvas items
    bool children_order_dirty;
    Color modulate;
    RID parent;                      // Parent canvas
    float parent_scale;
    
    HashSet<RendererCanvasRender::Light *> lights;
    HashSet<RendererCanvasRender::Light *> directional_lights;
    HashSet<RendererCanvasRender::LightOccluderInstance *> occluders;
};
```

### 10.4 Draw Commands (in RendererCanvasRender::Item)
Canvas items store a list of draw commands:
```cpp
// Base command types stored per-item
void canvas_item_add_line(RID p_item, const Point2 &from, const Point2 &to, ...);
void canvas_item_add_rect(RID p_item, const Rect2 &rect, const Color &color);
void canvas_item_add_circle(RID p_item, const Point2 &pos, float radius, ...);
void canvas_item_add_texture_rect(RID p_item, const Rect2 &rect, RID texture, ...);
void canvas_item_add_nine_patch(RID p_item, ...);  // 9-slice textures
void canvas_item_add_polygon(RID p_item, const Vector<Point2> &points, ...);
void canvas_item_add_triangle_array(RID p_item, ...);
void canvas_item_add_mesh(RID p_item, RID mesh, ...);
void canvas_item_add_multimesh(RID p_item, RID multimesh, ...);
void canvas_item_add_particles(RID p_item, RID particles, ...);
void canvas_item_add_set_transform(RID p_item, const Transform2D &xform);
void canvas_item_add_clip_ignore(RID p_item, bool ignore);
void canvas_item_clear(RID p_item);  // Clear all commands
```

### 10.5 Rendering Pipeline Flow
```
1. Collect dirty items (_item_update_list)
2. Sort canvas children by index
3. Cull items (_cull_canvas_item)
4. Build Z-sorted lists (z_list, z_last_list)
5. Optionally Y-sort children
6. Render via RendererCanvasRender backend
```

### 10.6 Z-Layer System
```cpp
static constexpr int z_range = RS::CANVAS_ITEM_Z_MAX - RS::CANVAS_ITEM_Z_MIN + 1;
// Z range: -4096 to 4096 (8193 layers)

// Z-lists are arrays indexed by z_index
RendererCanvasRender::Item **z_list;
RendererCanvasRender::Item **z_last_list;
```

### 10.7 Physics Interpolation
```cpp
struct InterpolationData {
    LocalVector<RID> canvas_item_transform_update_lists[2];  // Double-buffered
    LocalVector<RID> canvas_light_transform_update_lists[2];
    LocalVector<RID> canvas_light_occluder_transform_update_lists[2];
    bool interpolation_enabled = false;
};

void canvas_item_set_interpolated(RID p_item, bool p_interpolated);
void canvas_item_reset_physics_interpolation(RID p_item);
void canvas_item_transform_physics_interpolation(RID p_item, const Transform2D &xform);
```

### 10.8 Key Concepts for Implementation
1. **RID-based handles**: All resources referenced by opaque handles
2. **Deferred updates**: Items queued for update, processed in batch
3. **Double-buffered transforms**: For smooth physics interpolation
4. **Command lists per item**: Draw commands stored, replayed at render time
5. **Z-partitioning**: Fast sort using bucket arrays per Z-layer

---

## 11. UI Editor Components (ControlEditorPlugin)

**Location**: `editor/scene/gui/control_editor_plugin.h/cpp`

### 11.1 Architecture Overview
Godot's UI editor consists of:
- **ControlEditorToolbar**: Main toolbar with layout tools
- **Inspector Plugins**: Custom property editors for Control properties
- **Preset Pickers**: Visual grid selectors for anchors and size flags

### 11.2 ControlEditorToolbar
```cpp
class ControlEditorToolbar : public HBoxContainer {
    EditorSelection *editor_selection;
    
    ControlEditorPopupButton *anchors_button;    // Anchor preset picker
    ControlEditorPopupButton *containers_button; // Size flags picker
    Button *anchor_mode_button;                  // Toggle anchor editing
    
    SizeFlagPresetPicker *container_h_picker;
    SizeFlagPresetPicker *container_v_picker;
    
    bool anchors_mode = false;
    
    // Undo/redo integration
    void _anchors_preset_selected(int p_preset);
    void _container_flags_selected(int p_flags, bool p_vertical);
};
```

### 11.3 Anchor Preset Picker (Visual Grid)
```cpp
class AnchorPresetPicker : public ControlEditorPresetPicker {
    // 16 preset buttons arranged in a 4x4 grid
    // Emits signal when preset selected
    
    // Presets:
    // Row 1: TopLeft, CenterTop, TopRight | TopWide
    // Row 2: CenterLeft, Center, CenterRight | HCenterWide
    // Row 3: BottomLeft, CenterBottom, BottomRight | BottomWide
    // Row 4: LeftWide, VCenterWide, RightWide | FullRect
    
    void _preset_button_pressed(int p_preset) {
        emit_signal("anchors_preset_selected", p_preset);
    }
};
```

### 11.4 Size Flag Preset Picker
```cpp
class SizeFlagPresetPicker : public ControlEditorPresetPicker {
    bool vertical;              // H or V axis
    CheckButton *expand_button; // Toggle expand flag
    
    // 4 shrink/fill presets + expand toggle
    // Buttons: ShrinkBegin, ShrinkCenter, ShrinkEnd | Fill
    // Checkbox: Expand
    
    void _preset_button_pressed(int p_preset) {
        int flags = p_preset;
        if (expand_button->is_pressed()) {
            flags |= SIZE_EXPAND;
        }
        emit_signal("size_flags_selected", flags);
    }
};
```

### 11.5 Inspector Property Editors
```cpp
class EditorPropertyAnchorsPreset : public EditorProperty {
    OptionButton *options;  // Dropdown with all presets + icons
    
    void update_property() override;
    void _option_selected(int p_which);
};

class EditorPropertySizeFlags : public EditorProperty {
    OptionButton *flag_presets;  // Preset dropdown
    CheckBox *flag_expand;       // Expand toggle
    VBoxContainer *flag_options; // Individual flag checkboxes
    Vector<CheckBox *> flag_checks;
    
    enum FlagPreset {
        SIZE_FLAGS_PRESET_FILL,
        SIZE_FLAGS_PRESET_SHRINK_BEGIN,
        SIZE_FLAGS_PRESET_SHRINK_CENTER,
        SIZE_FLAGS_PRESET_SHRINK_END,
        SIZE_FLAGS_PRESET_CUSTOM,
    };
};
```

### 11.6 Positioning Warning Panel
```cpp
class ControlPositioningWarning : public MarginContainer {
    Control *control_node;
    TextureRect *title_icon;
    Label *title_label;
    Label *hint_label;
    
    void _update_warning() {
        Node *parent = control_node->get_parent_control();
        if (!parent) {
            // "This node doesn't have a control parent"
        } else if (Object::cast_to<Container>(parent)) {
            // "This node is a child of a container"
        } else {
            // "This node is a child of a regular control"
        }
    }
};
```

### 11.7 Undo/Redo Integration
```cpp
void ControlEditorToolbar::_anchors_preset_selected(int p_preset) {
    LayoutPreset preset = (LayoutPreset)p_preset;
    List<Node *> selection = editor_selection->get_top_selected_node_list();

    EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
    undo_redo->create_action(TTR("Change Anchors, Offsets, Grow Direction"));

    for (Node *E : selection) {
        Control *control = Object::cast_to<Control>(E);
        if (control) {
            undo_redo->add_do_property(control, "layout_mode", LAYOUT_MODE_ANCHORS);
            undo_redo->add_do_property(control, "anchors_preset", preset);
            undo_redo->add_undo_method(control, "_edit_set_state", control->_edit_get_state());
        }
    }
    undo_redo->commit_action();
}
```

### 11.8 Key UI Editor Features for Implementation
1. **Visual Anchor Grid**: 4x4 button grid with icons
2. **Size Flags Presets**: Quick access to common configurations
3. **Context-Aware Hints**: Shows different advice based on parent type
4. **Multi-Selection Support**: Apply to all selected controls
5. **Undo/Redo**: Full state capture before changes


---

## 12. File Structure Reference

```
scene/
├── gui/
│   ├── control.h/cpp           # Base Control class
│   ├── container.h/cpp         # Base Container class
│   ├── box_container.h/cpp     # VBox/HBox layout
│   ├── base_button.h/cpp       # Button base class
│   ├── button.h/cpp            # Standard button
│   ├── label.h/cpp             # Text label
│   ├── panel.h/cpp             # Simple panel
│   ├── scroll_container.h/cpp  # Scrollable area
│   └── ...
├── main/
│   └── canvas_item.h/cpp       # 2D rendering base
├── theme/
│   ├── theme_db.h/cpp          # Theme singleton
│   ├── theme_owner.h/cpp       # Theme propagation
│   └── default_theme.cpp       # Built-in theme
└── resources/
    ├── theme.h/cpp             # Theme resource
    ├── style_box.h/cpp         # Background styles
    ├── font.h/cpp              # Font resource
    └── texture.h/cpp           # Texture resource

servers/rendering/
├── renderer_canvas_cull.h/cpp  # Retained mode canvas rendering
├── renderer_canvas_render.h    # Backend rendering interface
└── rendering_server.h/cpp      # Main rendering API

editor/scene/gui/
├── control_editor_plugin.h/cpp # UI editing tools
└── canvas_item_editor.h/cpp    # 2D scene editor
```
