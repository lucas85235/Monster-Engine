#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "engine/ui/native/NativeUiRenderer.h"

namespace se {
class InputManager;
}

namespace se::ui::retained {

using UiId = uint64_t;
constexpr UiId kInvalidId = 0;

#define SE_RETAINED_UI_COMPONENT_LIST(X) \
    X(TextInput)                         \
    X(TextArea)                          \
    X(PasswordInput)                     \
    X(SearchInput)                       \
    X(RichTextEditor)                    \
    X(CodeEditor)                        \
    X(AutoCompleteInput)                 \
    X(Checkbox)                          \
    X(RadioButton)                       \
    X(ToggleSwitch)                      \
    X(DropdownSelect)                    \
    X(MultiSelect)                       \
    X(ComboBox)                          \
    X(ColorPicker)                       \
    X(DatePicker)                        \
    X(TimePicker)                        \
    X(DateRangePicker)                   \
    X(FilePicker)                        \
    X(Slider)                            \
    X(RangeSlider)                       \
    X(NumberInput)                       \
    X(Knob)                              \
    X(ProgressBar)                       \
    X(Rating)                            \
    X(Button)                            \
    X(IconButton)                        \
    X(FloatingActionButton)              \
    X(SplitButton)                       \
    X(ButtonGroup)                       \
    X(Link)                              \
    X(Tabs)                              \
    X(Breadcrumb)                        \
    X(Pagination)                        \
    X(Stepper)                           \
    X(Sidebar)                           \
    X(Navbar)                            \
    X(Menu)                              \
    X(ContextMenu)                       \
    X(BottomNavigation)                  \
    X(CommandPalette)                    \
    X(Label)                             \
    X(Badge)                             \
    X(Avatar)                            \
    X(Icon)                              \
    X(Tooltip)                           \
    X(Table)                             \
    X(DataGrid)                          \
    X(List)                              \
    X(TreeView)                          \
    X(Card)                              \
    X(Accordion)                         \
    X(Timeline)                          \
    X(StatCard)                          \
    X(EmptyState)                        \
    X(Alert)                             \
    X(Toast)                             \
    X(Notification)                      \
    X(Spinner)                           \
    X(SkeletonLoader)                    \
    X(InlineValidationMessage)           \
    X(Modal)                             \
    X(ConfirmationDialog)                \
    X(Popover)                           \
    X(Sheet)                             \
    X(Lightbox)                          \
    X(OverlayBackdrop)                   \
    X(Divider)                           \
    X(Spacer)                            \
    X(Grid)                              \
    X(HStack)                            \
    X(VStack)                            \
    X(ScrollArea)                        \
    X(ResizablePanel)                    \
    X(Splitter)                          \
    X(Image)                             \
    X(VideoPlayer)                       \
    X(AudioPlayer)                       \
    X(Carousel)                          \
    X(MapView)                           \
    X(Form)                              \
    X(WizardForm)                        \
    X(DragDropList)                      \
    X(VirtualList)                       \
    X(Calendar)                          \
    X(KanbanBoard)                       \
    X(ChatMessage)                       \
    X(CodeBlock)                         \
    X(DiffViewer)

enum class LayoutMode : uint8_t {
    Free = 0,
    VStack,
    HStack,
    Grid,
    Dock,
};

enum class DockSlot : uint8_t {
    None = 0,
    Left,
    Right,
    Top,
    Bottom,
    Fill,
    Floating,
};

enum class WidgetType : uint16_t {
    Root = 0,
    Window,
    Panel,
#define SE_WIDGET_ENUM(name) name,
    SE_RETAINED_UI_COMPONENT_LIST(SE_WIDGET_ENUM)
#undef SE_WIDGET_ENUM
};

enum class DirtyFlag : uint8_t {
    None   = 0,
    Layout = 1 << 0,
    Paint  = 1 << 1,
    Data   = 1 << 2,
    Tree   = 1 << 3,
};

inline DirtyFlag operator|(DirtyFlag a, DirtyFlag b) {
    return static_cast<DirtyFlag>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline DirtyFlag operator&(DirtyFlag a, DirtyFlag b) {
    return static_cast<DirtyFlag>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline DirtyFlag& operator|=(DirtyFlag& a, DirtyFlag b) {
    a = a | b;
    return a;
}

inline bool Any(DirtyFlag flags, DirtyFlag mask) {
    return (flags & mask) != DirtyFlag::None;
}

struct UiRect {
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

struct LayoutStyle {
    LayoutMode mode = LayoutMode::Free;
    DockSlot   dock = DockSlot::None;

    float x = 0.0f;
    float y = 0.0f;
    float width = 220.0f;
    float height = 36.0f;

    float minWidth = 24.0f;
    float minHeight = 20.0f;
    float maxWidth = 100000.0f;
    float maxHeight = 100000.0f;

    float padding = 8.0f;
    float spacing = 6.0f;
    float flexGrow = 1.0f;

    float anchorLeft = 0.0f;
    float anchorTop = 0.0f;
    float anchorRight = 0.0f;
    float anchorBottom = 0.0f;

    int  gridColumns = 2;
    int  zIndex = 0;
    bool fillX = true;
    bool fillY = false;
    bool visible = true;
    bool interactable = false;
    bool clipChildren = false;
    bool resizable = false;
};

struct VisualStyle {
    NativeUiColor background {0.08f, 0.11f, 0.15f, 0.94f};
    NativeUiColor border     {0.20f, 0.30f, 0.42f, 1.0f};
    NativeUiColor text       {0.90f, 0.95f, 1.0f, 1.0f};
    NativeUiColor accent     {0.28f, 0.53f, 0.84f, 1.0f};
    NativeUiColor disabled   {0.45f, 0.50f, 0.56f, 0.65f};

    float borderThickness = 1.0f;
    float cornerRadius = 0.0f;
    float fontScale = 1.0f;
    float opacity = 1.0f;
};

struct WidgetState {
    std::string title;
    std::string text;
    std::string valueText;
    std::string placeholder;

    std::vector<std::string> items;
    std::vector<std::string> rows;
    std::vector<std::string> columns;

    int selectedIndex = -1;
    std::vector<int> selectedIndices;

    float value = 0.0f;
    float secondaryValue = 1.0f;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float step = 1.0f;

    int ratingValue = 0;
    int ratingMax = 5;

    bool checked = false;
    bool open = false;
    bool multiline = false;
    bool password = false;
    bool searchEnabled = false;
    bool readOnly = false;
    bool draggable = false;
    bool hovered = false;
    bool pressed = false;
    bool focused = false;

    float scrollX = 0.0f;
    float scrollY = 0.0f;
    float splitRatio = 0.5f;
};

struct UiNode {
    UiId       id = kInvalidId;
    WidgetType type = WidgetType::Panel;
    UiId       parent = kInvalidId;
    std::vector<UiId> children;

    LayoutStyle layout;
    VisualStyle style;
    WidgetState state;

    UiRect worldRect;

    DirtyFlag dirty = DirtyFlag::Layout | DirtyFlag::Paint | DirtyFlag::Data;

    bool enabled = true;

    uint64_t dataVersion = 1;
    uint64_t layoutVersion = 1;
    uint64_t paintVersion = 1;
};

enum class DrawCommandType : uint8_t {
    FilledRect = 0,
    Rect,
    Text,
};

struct DrawCommand {
    DrawCommandType type = DrawCommandType::FilledRect;
    UiRect          rect;
    NativeUiColor   color;

    float thickness = 1.0f;
    float textScale = 1.0f;
    float textX = 0.0f;
    float textY = 0.0f;

    int zIndex = 0;
    std::string text;
};

struct RetainedUiStats {
    size_t nodeCount = 0;
    size_t drawCommandCount = 0;
    uint64_t layoutPasses = 0;
    uint64_t paintPasses = 0;
    uint64_t reusedFrames = 0;
    uint64_t uploadedFrames = 0;
    uint64_t dirtyFrames = 0;
};

class RetainedUiContext {
   public:
    static RetainedUiContext& Get();

    void Init();
    void Shutdown();

    bool IsInitialized() const {
        return initialized_;
    }

    void Reset();
    void SetViewport(float width, float height);

    UiNode& EnsureComponent(UiId id, WidgetType type, UiId parent = kInvalidId,
                            std::string_view text = {});

#define SE_DECLARE_ENSURE(name) \
    UiNode& Ensure##name(UiId id, UiId parent = kInvalidId, std::string_view text = {});
    SE_RETAINED_UI_COMPONENT_LIST(SE_DECLARE_ENSURE)
#undef SE_DECLARE_ENSURE

    UiNode& EnsureWindow(UiId id, UiId parent = kInvalidId, std::string_view title = {});
    UiNode& EnsurePanel(UiId id, UiId parent = kInvalidId, std::string_view title = {});

    bool RemoveComponent(UiId id);

    UiNode*       FindComponent(UiId id);
    const UiNode* FindComponent(UiId id) const;

    UiNode*       GetRoot();
    const UiNode* GetRoot() const;

    void SetText(UiId id, std::string_view text);
    void SetValueText(UiId id, std::string_view valueText);
    void SetPlaceholder(UiId id, std::string_view placeholder);
    void SetItems(UiId id, const std::vector<std::string>& items);

    void SetSelectedIndex(UiId id, int index);
    void SetValue(UiId id, float value);
    void SetSecondaryValue(UiId id, float value);
    void SetRange(UiId id, float minValue, float maxValue);
    void SetChecked(UiId id, bool checked);
    void SetOpen(UiId id, bool open);
    void SetFocused(UiId id, bool focused);

    void SetLayout(UiId id, const LayoutStyle& layout);
    void SetStyle(UiId id, const VisualStyle& style);

    void MarkDirty(UiId id, DirtyFlag flags = DirtyFlag::Data);

    void PollInput(se::InputManager& input);
    void Tick(float deltaSeconds);
    void Render(NativeUiRenderer& renderer);

    const RetainedUiStats& GetStats() const {
        return stats_;
    }

    bool HasUserContent() const {
        return nodes_.size() > 1;
    }

   private:
    RetainedUiContext() = default;

    UiNode& EnsureInternal(UiId id, WidgetType type, UiId parent, std::string_view text,
                           bool forceTitle);

    void BuildLayoutIfNeeded();
    void BuildPaintCacheIfNeeded();

    void LayoutNode(UiNode& node, const UiRect& parentRect);
    void LayoutChildrenDock(UiNode& node, const UiRect& contentRect);
    void LayoutChildrenStack(UiNode& node, const UiRect& contentRect, bool vertical);
    void LayoutChildrenGrid(UiNode& node, const UiRect& contentRect);
    void LayoutChildrenFree(UiNode& node, const UiRect& contentRect);

    void EmitNodePaint(const UiNode& node);
    void EmitGenericWidget(const UiNode& node, std::string_view fallbackLabel);

    UiRect ContentRect(const UiNode& node) const;

    UiNode*       FindTopInteractiveAt(float x, float y);
    const UiNode* FindTopInteractiveAt(float x, float y) const;

    void PropagateDirtyToRoot(UiNode& node, DirtyFlag flags);
    void MarkTreeDirty();

    static UiRect ClampRect(const UiRect& rect, const LayoutStyle& layout);
    static float  ClampFloat(float value, float minValue, float maxValue);

    bool initialized_ = false;

    UiId rootId_ = 1;

    std::unordered_map<UiId, UiNode> nodes_;

    float viewportWidth_ = 1.0f;
    float viewportHeight_ = 1.0f;

    std::vector<DrawCommand> drawCommands_;

    bool treeDirty_ = true;
    bool layoutDirty_ = true;
    bool paintDirty_ = true;

    UiId hoveredId_ = kInvalidId;
    UiId pressedId_ = kInvalidId;
    UiId focusedId_ = kInvalidId;
    UiId draggingId_ = kInvalidId;
    UiId resizingId_ = kInvalidId;

    float dragOffsetX_ = 0.0f;
    float dragOffsetY_ = 0.0f;
    float resizeStartMouseX_ = 0.0f;
    float resizeStartMouseY_ = 0.0f;
    float resizeStartWidth_  = 0.0f;
    float resizeStartHeight_ = 0.0f;

    RetainedUiStats stats_;
};

#undef SE_RETAINED_UI_COMPONENT_LIST

}  // namespace se::ui::retained
