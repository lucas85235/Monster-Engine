#include "engine/ui/native/retained/RetainedUi.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>

#include "engine/input/InputManager.h"
#include "engine/input/KeyCodes.h"

namespace se::ui::retained {

namespace {

constexpr float kEpsilon = 0.0001f;

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

bool IsContainerType(WidgetType type) {
    switch (type) {
        case WidgetType::Root:
        case WidgetType::Window:
        case WidgetType::Panel:
        case WidgetType::Sidebar:
        case WidgetType::Navbar:
        case WidgetType::Menu:
        case WidgetType::ContextMenu:
        case WidgetType::BottomNavigation:
        case WidgetType::CommandPalette:
        case WidgetType::Card:
        case WidgetType::Accordion:
        case WidgetType::Timeline:
        case WidgetType::StatCard:
        case WidgetType::EmptyState:
        case WidgetType::Modal:
        case WidgetType::ConfirmationDialog:
        case WidgetType::Popover:
        case WidgetType::Sheet:
        case WidgetType::Lightbox:
        case WidgetType::OverlayBackdrop:
        case WidgetType::Grid:
        case WidgetType::HStack:
        case WidgetType::VStack:
        case WidgetType::ScrollArea:
        case WidgetType::ResizablePanel:
        case WidgetType::Splitter:
        case WidgetType::Table:
        case WidgetType::DataGrid:
        case WidgetType::List:
        case WidgetType::TreeView:
        case WidgetType::Form:
        case WidgetType::WizardForm:
        case WidgetType::DragDropList:
        case WidgetType::VirtualList:
        case WidgetType::Calendar:
        case WidgetType::KanbanBoard:
        case WidgetType::Carousel:
        case WidgetType::MapView:
        case WidgetType::VideoPlayer:
        case WidgetType::AudioPlayer:
            return true;
        default:
            return false;
    }
}

bool IsInteractiveType(WidgetType type) {
    switch (type) {
        case WidgetType::TextInput:
        case WidgetType::TextArea:
        case WidgetType::PasswordInput:
        case WidgetType::SearchInput:
        case WidgetType::RichTextEditor:
        case WidgetType::CodeEditor:
        case WidgetType::AutoCompleteInput:
        case WidgetType::Checkbox:
        case WidgetType::RadioButton:
        case WidgetType::ToggleSwitch:
        case WidgetType::DropdownSelect:
        case WidgetType::MultiSelect:
        case WidgetType::ComboBox:
        case WidgetType::ColorPicker:
        case WidgetType::DatePicker:
        case WidgetType::TimePicker:
        case WidgetType::DateRangePicker:
        case WidgetType::FilePicker:
        case WidgetType::Slider:
        case WidgetType::RangeSlider:
        case WidgetType::NumberInput:
        case WidgetType::Knob:
        case WidgetType::Rating:
        case WidgetType::Button:
        case WidgetType::IconButton:
        case WidgetType::FloatingActionButton:
        case WidgetType::SplitButton:
        case WidgetType::ButtonGroup:
        case WidgetType::Link:
        case WidgetType::Tabs:
        case WidgetType::Breadcrumb:
        case WidgetType::Pagination:
        case WidgetType::Stepper:
        case WidgetType::Menu:
        case WidgetType::ContextMenu:
        case WidgetType::BottomNavigation:
        case WidgetType::CommandPalette:
        case WidgetType::Tooltip:
        case WidgetType::Table:
        case WidgetType::DataGrid:
        case WidgetType::List:
        case WidgetType::TreeView:
        case WidgetType::Accordion:
        case WidgetType::Modal:
        case WidgetType::ConfirmationDialog:
        case WidgetType::Popover:
        case WidgetType::Sheet:
        case WidgetType::Lightbox:
        case WidgetType::ResizablePanel:
        case WidgetType::Splitter:
        case WidgetType::Image:
        case WidgetType::VideoPlayer:
        case WidgetType::AudioPlayer:
        case WidgetType::Carousel:
        case WidgetType::MapView:
        case WidgetType::WizardForm:
        case WidgetType::DragDropList:
        case WidgetType::VirtualList:
        case WidgetType::Calendar:
        case WidgetType::KanbanBoard:
        case WidgetType::ChatMessage:
        case WidgetType::CodeBlock:
        case WidgetType::DiffViewer:
            return true;
        default:
            return false;
    }
}

bool IsTextEntryType(WidgetType type) {
    switch (type) {
        case WidgetType::TextInput:
        case WidgetType::TextArea:
        case WidgetType::PasswordInput:
        case WidgetType::SearchInput:
        case WidgetType::RichTextEditor:
        case WidgetType::CodeEditor:
        case WidgetType::AutoCompleteInput:
            return true;
        default:
            return false;
    }
}

const char* WidgetTypeName(WidgetType type) {
    switch (type) {
        case WidgetType::Root:
            return "Root";
        case WidgetType::Window:
            return "Window";
        case WidgetType::Panel:
            return "Panel";
#define SE_WIDGET_NAME(name) \
    case WidgetType::name:    \
        return #name;
            SE_RETAINED_UI_COMPONENT_LIST(SE_WIDGET_NAME)
#undef SE_WIDGET_NAME
        default:
            return "Widget";
    }
}

UiRect DefaultRect(WidgetType type) {
    UiRect rect{0.0f, 0.0f, 220.0f, 36.0f};
    switch (type) {
        case WidgetType::Window:
            rect.w = 840.0f;
            rect.h = 520.0f;
            break;
        case WidgetType::Panel:
        case WidgetType::Card:
        case WidgetType::ResizablePanel:
            rect.h = 220.0f;
            break;
        case WidgetType::TextArea:
        case WidgetType::RichTextEditor:
        case WidgetType::CodeEditor:
            rect.h = 140.0f;
            break;
        case WidgetType::Table:
        case WidgetType::DataGrid:
        case WidgetType::List:
        case WidgetType::TreeView:
        case WidgetType::Calendar:
        case WidgetType::KanbanBoard:
        case WidgetType::VirtualList:
            rect.w = 460.0f;
            rect.h = 280.0f;
            break;
        case WidgetType::Modal:
        case WidgetType::ConfirmationDialog:
        case WidgetType::Popover:
        case WidgetType::Sheet:
            rect.w = 460.0f;
            rect.h = 300.0f;
            break;
        case WidgetType::OverlayBackdrop:
            rect.w = 1920.0f;
            rect.h = 1080.0f;
            break;
        case WidgetType::ProgressBar:
            rect.h = 18.0f;
            break;
        case WidgetType::Divider:
            rect.h = 1.0f;
            break;
        case WidgetType::Spacer:
            rect.h = 10.0f;
            break;
        default:
            break;
    }
    return rect;
}

float DefaultMinHeight(WidgetType type) {
    switch (type) {
        case WidgetType::Divider:
            return 1.0f;
        case WidgetType::Spacer:
            return 2.0f;
        case WidgetType::ProgressBar:
            return 12.0f;
        case WidgetType::TextArea:
        case WidgetType::RichTextEditor:
        case WidgetType::CodeEditor:
            return 64.0f;
        default:
            return 20.0f;
    }
}

void ApplyAnchors(const LayoutStyle& layout, const UiRect& parentContent, UiRect* rect) {
    if (!rect) return;

    const bool anchoredX = layout.anchorRight > (layout.anchorLeft + kEpsilon);
    if (anchoredX) {
        const float left = parentContent.x + parentContent.w * layout.anchorLeft + layout.x;
        const float right = parentContent.x + parentContent.w * layout.anchorRight - layout.x;
        rect->x = left;
        rect->w = std::max(0.0f, right - left);
    }

    const bool anchoredY = layout.anchorBottom > (layout.anchorTop + kEpsilon);
    if (anchoredY) {
        const float top = parentContent.y + parentContent.h * layout.anchorTop + layout.y;
        const float bottom = parentContent.y + parentContent.h * layout.anchorBottom - layout.y;
        rect->y = top;
        rect->h = std::max(0.0f, bottom - top);
    }
}

}  // namespace

RetainedUiContext& RetainedUiContext::Get() {
    static RetainedUiContext instance;
    return instance;
}

void RetainedUiContext::Init() {
    if (initialized_) return;
    initialized_ = true;
    Reset();
}

void RetainedUiContext::Shutdown() {
    nodes_.clear();
    drawCommands_.clear();
    initialized_ = false;
}

void RetainedUiContext::Reset() {
    nodes_.clear();
    drawCommands_.clear();
    hoveredId_ = kInvalidId;
    pressedId_ = kInvalidId;
    focusedId_ = kInvalidId;
    draggingId_ = kInvalidId;
    resizingId_ = kInvalidId;

    UiNode root;
    root.id = rootId_;
    root.type = WidgetType::Root;
    root.parent = kInvalidId;
    root.layout.mode = LayoutMode::Dock;
    root.layout.fillX = true;
    root.layout.fillY = true;
    root.layout.width = viewportWidth_;
    root.layout.height = viewportHeight_;
    root.layout.padding = 0.0f;
    root.layout.spacing = 0.0f;
    root.layout.visible = true;
    root.layout.interactable = false;
    root.worldRect = {0.0f, 0.0f, viewportWidth_, viewportHeight_};
    root.enabled = true;

    nodes_.emplace(root.id, std::move(root));

    treeDirty_ = true;
    layoutDirty_ = true;
    paintDirty_ = true;

    stats_ = RetainedUiStats{};
    stats_.nodeCount = nodes_.size();
}

void RetainedUiContext::SetViewport(float width, float height) {
    viewportWidth_ = std::max(1.0f, width);
    viewportHeight_ = std::max(1.0f, height);

    if (UiNode* root = FindComponent(rootId_)) {
        root->layout.width = viewportWidth_;
        root->layout.height = viewportHeight_;
    }

    layoutDirty_ = true;
    paintDirty_ = true;
}

UiNode& RetainedUiContext::EnsureComponent(UiId id, WidgetType type, UiId parent,
                                           std::string_view text) {
    return EnsureInternal(id, type, parent, text, false);
}

UiNode& RetainedUiContext::EnsureWindow(UiId id, UiId parent, std::string_view title) {
    return EnsureInternal(id, WidgetType::Window, parent, title, true);
}

UiNode& RetainedUiContext::EnsurePanel(UiId id, UiId parent, std::string_view title) {
    return EnsureInternal(id, WidgetType::Panel, parent, title, true);
}

#define SE_DEFINE_ENSURE(name)                                                        \
    UiNode& RetainedUiContext::Ensure##name(UiId id, UiId parent, std::string_view text) { \
        return EnsureInternal(id, WidgetType::name, parent, text, false);             \
    }
SE_RETAINED_UI_COMPONENT_LIST(SE_DEFINE_ENSURE)
#undef SE_DEFINE_ENSURE

UiNode& RetainedUiContext::EnsureInternal(UiId id, WidgetType type, UiId parent,
                                          std::string_view text, bool forceTitle) {
    if (!initialized_) {
        Init();
    }

    if (id == kInvalidId) {
        id = static_cast<UiId>(nodes_.size()) + 1024;
    }

    UiId targetParent = parent;
    if (targetParent == kInvalidId && id != rootId_) {
        targetParent = rootId_;
    }

    auto [it, inserted] = nodes_.try_emplace(id);
    UiNode& node = it->second;

    if (inserted) {
        node.id = id;
        node.type = type;
        node.parent = kInvalidId;

        const UiRect defRect = DefaultRect(type);
        node.layout.width = defRect.w;
        node.layout.height = defRect.h;
        node.layout.minHeight = DefaultMinHeight(type);
        node.layout.minWidth = 24.0f;
        node.layout.fillX = true;
        node.layout.fillY = false;
        node.layout.interactable = IsInteractiveType(type);
        node.layout.visible = true;

        node.enabled = true;

        if (type == WidgetType::Window || type == WidgetType::Modal ||
            type == WidgetType::ConfirmationDialog || type == WidgetType::Popover ||
            type == WidgetType::Sheet || type == WidgetType::ResizablePanel) {
            node.state.draggable = true;
            node.layout.resizable = true;
            node.layout.interactable = true;
            node.layout.fillX = false;
            node.layout.fillY = false;
        }

        if (IsContainerType(type)) {
            if (type == WidgetType::HStack) node.layout.mode = LayoutMode::HStack;
            if (type == WidgetType::VStack) node.layout.mode = LayoutMode::VStack;
            if (type == WidgetType::Grid) {
                node.layout.mode = LayoutMode::Grid;
                node.layout.gridColumns = 2;
            }
            if (type == WidgetType::Window || type == WidgetType::Panel || type == WidgetType::Card ||
                type == WidgetType::Modal || type == WidgetType::ConfirmationDialog ||
                type == WidgetType::Popover || type == WidgetType::Sheet ||
                type == WidgetType::ResizablePanel || type == WidgetType::ScrollArea ||
                type == WidgetType::Sidebar || type == WidgetType::Navbar ||
                type == WidgetType::Menu || type == WidgetType::ContextMenu ||
                type == WidgetType::CommandPalette || type == WidgetType::Form ||
                type == WidgetType::WizardForm) {
                node.layout.mode = LayoutMode::VStack;
            }
        }

        if (type == WidgetType::TextArea || type == WidgetType::RichTextEditor ||
            type == WidgetType::CodeEditor) {
            node.state.multiline = true;
        }
        if (type == WidgetType::PasswordInput) {
            node.state.password = true;
        }
        if (type == WidgetType::SearchInput || type == WidgetType::ComboBox ||
            type == WidgetType::CommandPalette || type == WidgetType::AutoCompleteInput) {
            node.state.searchEnabled = true;
        }

        if (type == WidgetType::Slider || type == WidgetType::RangeSlider ||
            type == WidgetType::NumberInput || type == WidgetType::Knob ||
            type == WidgetType::ProgressBar || type == WidgetType::Rating) {
            node.state.minValue = 0.0f;
            node.state.maxValue = 1.0f;
            node.state.value = 0.0f;
            node.state.secondaryValue = 1.0f;
        }

        node.dirty = DirtyFlag::Layout | DirtyFlag::Paint | DirtyFlag::Data;
        treeDirty_ = true;
        layoutDirty_ = true;
        paintDirty_ = true;
    } else if (node.type != type) {
        node.type = type;
        node.dirty |= DirtyFlag::Layout | DirtyFlag::Paint | DirtyFlag::Data;
        layoutDirty_ = true;
        paintDirty_ = true;
    }

    if (!text.empty()) {
        if (forceTitle) {
            if (node.state.title != text) {
                node.state.title.assign(text.begin(), text.end());
                node.dirty |= DirtyFlag::Paint | DirtyFlag::Data;
                node.dataVersion++;
                paintDirty_ = true;
            }
        } else if (node.state.text != text) {
            node.state.text.assign(text.begin(), text.end());
            node.dirty |= DirtyFlag::Paint | DirtyFlag::Data;
            node.dataVersion++;
            paintDirty_ = true;
        }
    }

    if (targetParent != kInvalidId && targetParent != node.id) {
        if (!nodes_.contains(targetParent)) {
            EnsureInternal(targetParent, WidgetType::Panel, rootId_, "", true);
        }

        if (node.parent != targetParent) {
            if (node.parent != kInvalidId) {
                if (UiNode* oldParent = FindComponent(node.parent)) {
                    oldParent->children.erase(
                        std::remove(oldParent->children.begin(), oldParent->children.end(), node.id),
                        oldParent->children.end());
                }
            }

            node.parent = targetParent;
            UiNode* newParent = FindComponent(targetParent);
            if (newParent &&
                std::find(newParent->children.begin(), newParent->children.end(), node.id) ==
                    newParent->children.end()) {
                newParent->children.push_back(node.id);
            }

            node.dirty |= DirtyFlag::Layout | DirtyFlag::Tree;
            treeDirty_ = true;
            layoutDirty_ = true;
            paintDirty_ = true;
        }
    }

    stats_.nodeCount = nodes_.size();
    return node;
}

bool RetainedUiContext::RemoveComponent(UiId id) {
    if (id == kInvalidId || id == rootId_) return false;

    UiNode* node = FindComponent(id);
    if (!node) return false;

    std::vector<UiId> toRemove;
    toRemove.push_back(id);
    for (size_t i = 0; i < toRemove.size(); ++i) {
        UiNode* current = FindComponent(toRemove[i]);
        if (!current) continue;
        for (UiId child : current->children) {
            toRemove.push_back(child);
        }
    }

    if (node->parent != kInvalidId) {
        if (UiNode* parent = FindComponent(node->parent)) {
            parent->children.erase(
                std::remove(parent->children.begin(), parent->children.end(), id),
                parent->children.end());
        }
    }

    for (UiId removeId : toRemove) {
        nodes_.erase(removeId);
    }

    if (hoveredId_ == id) hoveredId_ = kInvalidId;
    if (pressedId_ == id) pressedId_ = kInvalidId;
    if (focusedId_ == id) focusedId_ = kInvalidId;

    treeDirty_ = true;
    layoutDirty_ = true;
    paintDirty_ = true;
    stats_.nodeCount = nodes_.size();

    return true;
}

UiNode* RetainedUiContext::FindComponent(UiId id) {
    auto it = nodes_.find(id);
    return it == nodes_.end() ? nullptr : &it->second;
}

const UiNode* RetainedUiContext::FindComponent(UiId id) const {
    auto it = nodes_.find(id);
    return it == nodes_.end() ? nullptr : &it->second;
}

UiNode* RetainedUiContext::GetRoot() {
    return FindComponent(rootId_);
}

const UiNode* RetainedUiContext::GetRoot() const {
    return FindComponent(rootId_);
}

void RetainedUiContext::SetText(UiId id, std::string_view text) {
    if (UiNode* node = FindComponent(id)) {
        if (node->state.text != text) {
            node->state.text.assign(text.begin(), text.end());
            MarkDirty(id, DirtyFlag::Data | DirtyFlag::Paint);
        }
    }
}

void RetainedUiContext::SetValueText(UiId id, std::string_view valueText) {
    if (UiNode* node = FindComponent(id)) {
        if (node->state.valueText != valueText) {
            node->state.valueText.assign(valueText.begin(), valueText.end());
            MarkDirty(id, DirtyFlag::Data | DirtyFlag::Paint);
        }
    }
}

void RetainedUiContext::SetPlaceholder(UiId id, std::string_view placeholder) {
    if (UiNode* node = FindComponent(id)) {
        if (node->state.placeholder != placeholder) {
            node->state.placeholder.assign(placeholder.begin(), placeholder.end());
            MarkDirty(id, DirtyFlag::Data | DirtyFlag::Paint);
        }
    }
}

void RetainedUiContext::SetItems(UiId id, const std::vector<std::string>& items) {
    if (UiNode* node = FindComponent(id)) {
        node->state.items = items;
        MarkDirty(id, DirtyFlag::Data | DirtyFlag::Paint | DirtyFlag::Layout);
    }
}

void RetainedUiContext::SetSelectedIndex(UiId id, int index) {
    if (UiNode* node = FindComponent(id)) {
        if (node->state.selectedIndex != index) {
            node->state.selectedIndex = index;
            MarkDirty(id, DirtyFlag::Data | DirtyFlag::Paint);
        }
    }
}

void RetainedUiContext::SetValue(UiId id, float value) {
    if (UiNode* node = FindComponent(id)) {
        const float clamped = ClampFloat(value, node->state.minValue, node->state.maxValue);
        if (std::abs(node->state.value - clamped) > kEpsilon) {
            node->state.value = clamped;
            MarkDirty(id, DirtyFlag::Data | DirtyFlag::Paint);
        }
    }
}

void RetainedUiContext::SetSecondaryValue(UiId id, float value) {
    if (UiNode* node = FindComponent(id)) {
        const float clamped = ClampFloat(value, node->state.minValue, node->state.maxValue);
        if (std::abs(node->state.secondaryValue - clamped) > kEpsilon) {
            node->state.secondaryValue = clamped;
            MarkDirty(id, DirtyFlag::Data | DirtyFlag::Paint);
        }
    }
}

void RetainedUiContext::SetRange(UiId id, float minValue, float maxValue) {
    if (UiNode* node = FindComponent(id)) {
        if (minValue > maxValue) std::swap(minValue, maxValue);
        if (std::abs(node->state.minValue - minValue) > kEpsilon ||
            std::abs(node->state.maxValue - maxValue) > kEpsilon) {
            node->state.minValue = minValue;
            node->state.maxValue = maxValue;
            node->state.value = ClampFloat(node->state.value, minValue, maxValue);
            node->state.secondaryValue = ClampFloat(node->state.secondaryValue, minValue, maxValue);
            MarkDirty(id, DirtyFlag::Data | DirtyFlag::Paint);
        }
    }
}

void RetainedUiContext::SetChecked(UiId id, bool checked) {
    if (UiNode* node = FindComponent(id)) {
        if (node->state.checked != checked) {
            node->state.checked = checked;
            MarkDirty(id, DirtyFlag::Data | DirtyFlag::Paint);
        }
    }
}

void RetainedUiContext::SetOpen(UiId id, bool open) {
    if (UiNode* node = FindComponent(id)) {
        if (node->state.open != open) {
            node->state.open = open;
            MarkDirty(id, DirtyFlag::Data | DirtyFlag::Paint | DirtyFlag::Layout);
        }
    }
}

void RetainedUiContext::SetFocused(UiId id, bool focused) {
    UiNode* node = FindComponent(id);
    if (!node) return;

    if (focused) {
        if (focusedId_ != kInvalidId && focusedId_ != id) {
            if (UiNode* prev = FindComponent(focusedId_)) {
                prev->state.focused = false;
                MarkDirty(prev->id, DirtyFlag::Paint);
            }
        }
        focusedId_ = id;
        if (!node->state.focused) {
            node->state.focused = true;
            MarkDirty(id, DirtyFlag::Paint);
        }
    } else if (focusedId_ == id) {
        focusedId_ = kInvalidId;
        if (node->state.focused) {
            node->state.focused = false;
            MarkDirty(id, DirtyFlag::Paint);
        }
    }
}

void RetainedUiContext::SetLayout(UiId id, const LayoutStyle& layout) {
    if (UiNode* node = FindComponent(id)) {
        node->layout = layout;
        MarkDirty(id, DirtyFlag::Layout | DirtyFlag::Paint);
    }
}

void RetainedUiContext::SetStyle(UiId id, const VisualStyle& style) {
    if (UiNode* node = FindComponent(id)) {
        node->style = style;
        MarkDirty(id, DirtyFlag::Paint);
    }
}

void RetainedUiContext::MarkDirty(UiId id, DirtyFlag flags) {
    UiNode* node = FindComponent(id);
    if (!node) return;

    PropagateDirtyToRoot(*node, flags);
}

void RetainedUiContext::PropagateDirtyToRoot(UiNode& node, DirtyFlag flags) {
    UiNode* current = &node;
    while (current) {
        current->dirty |= flags;
        if (Any(flags, DirtyFlag::Data)) current->dataVersion++;
        if (Any(flags, DirtyFlag::Layout)) current->layoutVersion++;
        if (Any(flags, DirtyFlag::Paint)) current->paintVersion++;

        if (current->parent == kInvalidId) break;
        current = FindComponent(current->parent);
    }

    if (Any(flags, DirtyFlag::Tree)) treeDirty_ = true;
    if (Any(flags, DirtyFlag::Layout)) layoutDirty_ = true;
    if (Any(flags, DirtyFlag::Paint) || Any(flags, DirtyFlag::Data)) paintDirty_ = true;
}

void RetainedUiContext::MarkTreeDirty() {
    treeDirty_ = true;
    layoutDirty_ = true;
    paintDirty_ = true;
}

void RetainedUiContext::PollInput(se::InputManager& input) {
    if (!initialized_) return;

    BuildLayoutIfNeeded();

    const auto mousePos = input.GetMousePosition();
    UiNode* hovered = FindTopInteractiveAt(mousePos.x, mousePos.y);
    UiId    hoveredNow = hovered ? hovered->id : kInvalidId;

    if (hoveredNow != hoveredId_) {
        if (UiNode* prev = FindComponent(hoveredId_)) {
            prev->state.hovered = false;
            MarkDirty(prev->id, DirtyFlag::Paint);
        }
        hoveredId_ = hoveredNow;
        if (hovered) {
            hovered->state.hovered = true;
            MarkDirty(hovered->id, DirtyFlag::Paint);
        }
    }

    const bool leftPressed  = input.IsMouseButtonJustPressed(Mouse::ButtonLeft);
    const bool leftDown     = input.IsMouseButtonDown(Mouse::ButtonLeft);
    const bool leftReleased = input.IsMouseButtonJustReleased(Mouse::ButtonLeft);
    const bool hadDrag      = draggingId_ != kInvalidId;
    const bool hadResize    = resizingId_ != kInvalidId;

    if (leftPressed) {
        pressedId_ = hoveredId_;
        if (UiNode* pressed = FindComponent(pressedId_)) {
            pressed->state.pressed = true;
            MarkDirty(pressed->id, DirtyFlag::Paint);

            constexpr float resizeGrip = 12.0f;
            const bool insideResizeGrip =
                pressed->layout.resizable &&
                mousePos.x >= (pressed->worldRect.Right() - resizeGrip) &&
                mousePos.y >= (pressed->worldRect.Bottom() - resizeGrip);

            if (insideResizeGrip) {
                resizingId_        = pressed->id;
                draggingId_        = kInvalidId;
                resizeStartMouseX_ = mousePos.x;
                resizeStartMouseY_ = mousePos.y;
                resizeStartWidth_  = pressed->layout.width;
                resizeStartHeight_ = pressed->layout.height;
            } else if (pressed->state.draggable) {
                draggingId_  = pressed->id;
                resizingId_  = kInvalidId;
                dragOffsetX_ = mousePos.x - pressed->worldRect.x;
                dragOffsetY_ = mousePos.y - pressed->worldRect.y;
            } else {
                draggingId_ = kInvalidId;
                resizingId_ = kInvalidId;
            }
        }

        if (hovered && IsTextEntryType(hovered->type)) {
            SetFocused(hovered->id, true);
        } else if (focusedId_ != kInvalidId) {
            SetFocused(focusedId_, false);
        }
    }

    if (draggingId_ != kInvalidId) {
        if (leftDown) {
            if (UiNode* node = FindComponent(draggingId_)) {
                UiRect parentContent{0.0f, 0.0f, viewportWidth_, viewportHeight_};
                if (node->parent != kInvalidId) {
                    if (const UiNode* parent = FindComponent(node->parent)) {
                        parentContent = ContentRect(*parent);
                    }
                }

                const float maxX = std::max(0.0f, parentContent.w - node->layout.width);
                const float maxY = std::max(0.0f, parentContent.h - node->layout.height);

                const float nextX = ClampFloat(mousePos.x - dragOffsetX_ - parentContent.x, 0.0f, maxX);
                const float nextY = ClampFloat(mousePos.y - dragOffsetY_ - parentContent.y, 0.0f, maxY);
                if (std::abs(node->layout.x - nextX) > kEpsilon ||
                    std::abs(node->layout.y - nextY) > kEpsilon) {
                    node->layout.x = nextX;
                    node->layout.y = nextY;
                    MarkDirty(node->id, DirtyFlag::Layout | DirtyFlag::Paint);
                }
            }
        } else {
            draggingId_ = kInvalidId;
        }
    }

    if (resizingId_ != kInvalidId) {
        if (leftDown) {
            if (UiNode* node = FindComponent(resizingId_)) {
                const float deltaX = mousePos.x - resizeStartMouseX_;
                const float deltaY = mousePos.y - resizeStartMouseY_;

                float nextWidth = resizeStartWidth_ + deltaX;
                float nextHeight = resizeStartHeight_ + deltaY;
                nextWidth = ClampFloat(nextWidth, node->layout.minWidth, node->layout.maxWidth);
                nextHeight = ClampFloat(nextHeight, node->layout.minHeight, node->layout.maxHeight);

                if (node->parent != kInvalidId) {
                    if (const UiNode* parent = FindComponent(node->parent)) {
                        const UiRect parentContent = ContentRect(*parent);
                        const float maxWidth = std::max(node->layout.minWidth,
                                                        parentContent.w - node->layout.x);
                        const float maxHeight = std::max(node->layout.minHeight,
                                                         parentContent.h - node->layout.y);
                        nextWidth = std::min(nextWidth, maxWidth);
                        nextHeight = std::min(nextHeight, maxHeight);
                    }
                }

                if (std::abs(node->layout.width - nextWidth) > kEpsilon ||
                    std::abs(node->layout.height - nextHeight) > kEpsilon) {
                    node->layout.width = nextWidth;
                    node->layout.height = nextHeight;
                    MarkDirty(node->id, DirtyFlag::Layout | DirtyFlag::Paint);
                }
            }
        } else {
            resizingId_ = kInvalidId;
        }
    }

    if (leftReleased) {
        UiId released = pressedId_;
        if (UiNode* pressed = FindComponent(released)) {
            pressed->state.pressed = false;
            MarkDirty(pressed->id, DirtyFlag::Paint);
        }

        if (!hadDrag && !hadResize && released != kInvalidId && released == hoveredId_) {
            UiNode* node = FindComponent(released);
            if (node) {
                switch (node->type) {
                    case WidgetType::Checkbox:
                    case WidgetType::ToggleSwitch:
                    case WidgetType::RadioButton:
                        node->state.checked = !node->state.checked;
                        MarkDirty(node->id, DirtyFlag::Data | DirtyFlag::Paint);
                        break;
                    case WidgetType::DropdownSelect:
                    case WidgetType::ComboBox:
                    case WidgetType::ContextMenu:
                    case WidgetType::Menu:
                    case WidgetType::Popover:
                    case WidgetType::Accordion:
                        node->state.open = !node->state.open;
                        MarkDirty(node->id, DirtyFlag::Data | DirtyFlag::Paint | DirtyFlag::Layout);
                        break;
                    default:
                        break;
                }
            }
        }

        pressedId_ = kInvalidId;
        draggingId_ = kInvalidId;
        resizingId_ = kInvalidId;
    }

    if (focusedId_ != kInvalidId) {
        UiNode* focused = FindComponent(focusedId_);
        if (focused && IsTextEntryType(focused->type) && !focused->state.readOnly) {
            for (uint32_t codepoint : input.ConsumeTextInput()) {
                if (codepoint == '\r') continue;
                if (codepoint == '\n' && !focused->state.multiline) continue;
                if (codepoint < 32 || codepoint > 126) continue;
                focused->state.valueText.push_back(static_cast<char>(codepoint));
                MarkDirty(focused->id, DirtyFlag::Data | DirtyFlag::Paint);
            }

            if (input.IsKeyJustPressed(Key::Backspace) && !focused->state.valueText.empty()) {
                focused->state.valueText.pop_back();
                MarkDirty(focused->id, DirtyFlag::Data | DirtyFlag::Paint);
            }
        } else {
            (void)input.ConsumeTextInput();
        }
    } else {
        (void)input.ConsumeTextInput();
    }
}

void RetainedUiContext::Tick(float /*deltaSeconds*/) {
    if (!initialized_) return;
    // Reserved for animation timers / caret blink / transitions.
}

void RetainedUiContext::Render(NativeUiRenderer& renderer) {
    if (!initialized_) return;

    const bool dirty = treeDirty_ || layoutDirty_ || paintDirty_;
    if (!dirty) {
        renderer.RetainPreviousFrameGeometry();
        stats_.reusedFrames++;
        return;
    }

    stats_.dirtyFrames++;

    BuildLayoutIfNeeded();
    BuildPaintCacheIfNeeded();

    renderer.InvalidateRetainedGeometry();

    for (const DrawCommand& cmd : drawCommands_) {
        switch (cmd.type) {
            case DrawCommandType::FilledRect:
                renderer.DrawFilledRect(cmd.rect.x, cmd.rect.y, cmd.rect.w, cmd.rect.h, cmd.color);
                break;
            case DrawCommandType::Rect:
                renderer.DrawRect(cmd.rect.x, cmd.rect.y, cmd.rect.w, cmd.rect.h, cmd.thickness,
                                  cmd.color);
                break;
            case DrawCommandType::Text:
                renderer.DrawText(cmd.text, cmd.textX, cmd.textY, cmd.color, cmd.textScale);
                break;
        }
    }

    stats_.uploadedFrames++;

    treeDirty_ = false;
    layoutDirty_ = false;
    paintDirty_ = false;

    for (auto& [id, node] : nodes_) {
        (void)id;
        node.dirty = DirtyFlag::None;
    }
}

void RetainedUiContext::BuildLayoutIfNeeded() {
    if (!layoutDirty_) return;

    UiNode* root = FindComponent(rootId_);
    if (!root) return;

    root->worldRect = UiRect{0.0f, 0.0f, viewportWidth_, viewportHeight_};
    LayoutNode(*root, root->worldRect);

    stats_.layoutPasses++;
    layoutDirty_ = false;
    paintDirty_ = true;
}

void RetainedUiContext::BuildPaintCacheIfNeeded() {
    if (!paintDirty_) return;

    drawCommands_.clear();

    std::function<void(const UiNode&)> emitRecursive = [&](const UiNode& node) {
        if (!node.layout.visible || node.style.opacity <= 0.0f) return;

        EmitNodePaint(node);
        for (UiId childId : node.children) {
            const UiNode* child = FindComponent(childId);
            if (!child) continue;
            emitRecursive(*child);
        }
    };

    if (const UiNode* root = FindComponent(rootId_)) {
        emitRecursive(*root);
    }

    std::stable_sort(drawCommands_.begin(), drawCommands_.end(),
                     [](const DrawCommand& a, const DrawCommand& b) {
                         return a.zIndex < b.zIndex;
                     });

    stats_.drawCommandCount = drawCommands_.size();
    stats_.paintPasses++;

    paintDirty_ = false;
}

UiRect RetainedUiContext::ClampRect(const UiRect& rect, const LayoutStyle& layout) {
    UiRect result = rect;
    result.w = ClampFloat(result.w, layout.minWidth, layout.maxWidth);
    result.h = ClampFloat(result.h, layout.minHeight, layout.maxHeight);
    return result;
}

float RetainedUiContext::ClampFloat(float value, float minValue, float maxValue) {
    return std::max(minValue, std::min(maxValue, value));
}

void RetainedUiContext::LayoutNode(UiNode& node, const UiRect& parentRect) {
    if (node.id == rootId_) {
        node.worldRect = parentRect;
    } else if (node.worldRect.w <= 0.0f || node.worldRect.h <= 0.0f ||
               Any(node.dirty, DirtyFlag::Layout)) {
        UiRect rect;
        rect.x = parentRect.x + node.layout.x;
        rect.y = parentRect.y + node.layout.y;
        rect.w = node.layout.fillX ? std::max(0.0f, parentRect.w - node.layout.x * 2.0f)
                                   : node.layout.width;
        rect.h = node.layout.fillY ? std::max(0.0f, parentRect.h - node.layout.y * 2.0f)
                                   : node.layout.height;
        node.worldRect = rect;
    }
    node.worldRect = ClampRect(node.worldRect, node.layout);

    const UiRect contentRect = ContentRect(node);

    switch (node.layout.mode) {
        case LayoutMode::Dock:
            LayoutChildrenDock(node, contentRect);
            break;
        case LayoutMode::VStack:
            LayoutChildrenStack(node, contentRect, true);
            break;
        case LayoutMode::HStack:
            LayoutChildrenStack(node, contentRect, false);
            break;
        case LayoutMode::Grid:
            LayoutChildrenGrid(node, contentRect);
            break;
        case LayoutMode::Free:
        default:
            LayoutChildrenFree(node, contentRect);
            break;
    }
}

UiRect RetainedUiContext::ContentRect(const UiNode& node) const {
    const float pad = std::max(0.0f, node.layout.padding);
    UiRect content = node.worldRect;
    content.x += pad;
    content.y += pad;
    content.w = std::max(0.0f, content.w - pad * 2.0f);
    content.h = std::max(0.0f, content.h - pad * 2.0f);
    return content;
}

void RetainedUiContext::LayoutChildrenDock(UiNode& node, const UiRect& contentRect) {
    UiRect remaining = contentRect;

    for (UiId childId : node.children) {
        UiNode* child = FindComponent(childId);
        if (!child || !child->layout.visible) continue;

        UiRect rect = child->worldRect;
        rect.w = child->layout.width;
        rect.h = child->layout.height;

        switch (child->layout.dock) {
            case DockSlot::Left:
                rect = UiRect{remaining.x, remaining.y, ClampFloat(rect.w, child->layout.minWidth,
                                                                   std::max(child->layout.minWidth, remaining.w)),
                              remaining.h};
                remaining.x += rect.w + node.layout.spacing;
                remaining.w = std::max(0.0f, remaining.w - rect.w - node.layout.spacing);
                break;
            case DockSlot::Right:
                rect.w = ClampFloat(rect.w, child->layout.minWidth,
                                    std::max(child->layout.minWidth, remaining.w));
                rect = UiRect{remaining.Right() - rect.w, remaining.y, rect.w, remaining.h};
                remaining.w = std::max(0.0f, remaining.w - rect.w - node.layout.spacing);
                break;
            case DockSlot::Top:
                rect = UiRect{remaining.x, remaining.y, remaining.w,
                              ClampFloat(rect.h, child->layout.minHeight,
                                         std::max(child->layout.minHeight, remaining.h))};
                remaining.y += rect.h + node.layout.spacing;
                remaining.h = std::max(0.0f, remaining.h - rect.h - node.layout.spacing);
                break;
            case DockSlot::Bottom:
                rect.h = ClampFloat(rect.h, child->layout.minHeight,
                                    std::max(child->layout.minHeight, remaining.h));
                rect = UiRect{remaining.x, remaining.Bottom() - rect.h, remaining.w, rect.h};
                remaining.h = std::max(0.0f, remaining.h - rect.h - node.layout.spacing);
                break;
            case DockSlot::Fill:
                rect = remaining;
                break;
            case DockSlot::Floating:
            case DockSlot::None:
            default:
                rect = UiRect{contentRect.x + child->layout.x, contentRect.y + child->layout.y,
                              child->layout.width, child->layout.height};
                if (child->layout.fillX) rect.w = contentRect.w - child->layout.x * 2.0f;
                if (child->layout.fillY) rect.h = contentRect.h - child->layout.y * 2.0f;
                ApplyAnchors(child->layout, contentRect, &rect);
                break;
        }

        child->worldRect = ClampRect(rect, child->layout);
        LayoutNode(*child, child->worldRect);
    }
}

void RetainedUiContext::LayoutChildrenStack(UiNode& node, const UiRect& contentRect, bool vertical) {
    std::vector<UiNode*> children;
    children.reserve(node.children.size());
    for (UiId childId : node.children) {
        UiNode* child = FindComponent(childId);
        if (!child || !child->layout.visible) continue;
        children.push_back(child);
    }

    if (children.empty()) return;

    const float spacing = std::max(0.0f, node.layout.spacing);
    const float availablePrimary =
        (vertical ? contentRect.h : contentRect.w) - spacing * static_cast<float>(children.size() - 1);

    float fixedPrimary = 0.0f;
    float growTotal = 0.0f;

    for (UiNode* child : children) {
        const bool expand = vertical ? child->layout.fillY : child->layout.fillX;
        if (expand || child->layout.flexGrow > 0.0f) {
            growTotal += std::max(0.0f, child->layout.flexGrow);
        } else {
            fixedPrimary += vertical ? child->layout.height : child->layout.width;
        }
    }

    const float remainingPrimary = std::max(0.0f, availablePrimary - fixedPrimary);

    float cursor = vertical ? contentRect.y : contentRect.x;

    for (UiNode* child : children) {
        const bool expand = vertical ? child->layout.fillY : child->layout.fillX;
        float primary = vertical ? child->layout.height : child->layout.width;

        if ((expand || child->layout.flexGrow > 0.0f) && growTotal > 0.0f) {
            const float ratio = std::max(0.0f, child->layout.flexGrow) / growTotal;
            primary = remainingPrimary * ratio;
        }

        UiRect rect;
        if (vertical) {
            rect.x = contentRect.x + child->layout.x;
            rect.y = cursor;
            rect.w = child->layout.fillX ? contentRect.w - child->layout.x * 2.0f : child->layout.width;
            rect.h = primary;
            cursor += rect.h + spacing;
        } else {
            rect.x = cursor;
            rect.y = contentRect.y + child->layout.y;
            rect.w = primary;
            rect.h = child->layout.fillY ? contentRect.h - child->layout.y * 2.0f : child->layout.height;
            cursor += rect.w + spacing;
        }
        ApplyAnchors(child->layout, contentRect, &rect);

        child->worldRect = ClampRect(rect, child->layout);
        LayoutNode(*child, child->worldRect);
    }
}

void RetainedUiContext::LayoutChildrenGrid(UiNode& node, const UiRect& contentRect) {
    std::vector<UiNode*> children;
    children.reserve(node.children.size());
    for (UiId childId : node.children) {
        UiNode* child = FindComponent(childId);
        if (!child || !child->layout.visible) continue;
        children.push_back(child);
    }

    if (children.empty()) return;

    const int cols = std::max(1, node.layout.gridColumns);
    const float spacing = std::max(0.0f, node.layout.spacing);
    const float cellW =
        (contentRect.w - spacing * static_cast<float>(std::max(0, cols - 1))) / static_cast<float>(cols);

    for (size_t i = 0; i < children.size(); ++i) {
        UiNode* child = children[i];
        const int row = static_cast<int>(i) / cols;
        const int col = static_cast<int>(i) % cols;

        UiRect rect;
        rect.x = contentRect.x + static_cast<float>(col) * (cellW + spacing) + child->layout.x;
        rect.y = contentRect.y + static_cast<float>(row) * (child->layout.height + spacing) +
                 child->layout.y;
        rect.w = child->layout.fillX ? cellW - child->layout.x * 2.0f : child->layout.width;
        rect.h = child->layout.height;
        ApplyAnchors(child->layout, contentRect, &rect);

        child->worldRect = ClampRect(rect, child->layout);
        LayoutNode(*child, child->worldRect);
    }
}

void RetainedUiContext::LayoutChildrenFree(UiNode& node, const UiRect& contentRect) {
    for (UiId childId : node.children) {
        UiNode* child = FindComponent(childId);
        if (!child || !child->layout.visible) continue;

        UiRect rect;
        rect.x = contentRect.x + child->layout.x;
        rect.y = contentRect.y + child->layout.y;
        rect.w = child->layout.fillX ? contentRect.w - child->layout.x * 2.0f : child->layout.width;
        rect.h = child->layout.fillY ? contentRect.h - child->layout.y * 2.0f : child->layout.height;
        ApplyAnchors(child->layout, contentRect, &rect);

        child->worldRect = ClampRect(rect, child->layout);
        LayoutNode(*child, child->worldRect);
    }
}

void RetainedUiContext::EmitNodePaint(const UiNode& node) {
    const UiRect rect = node.worldRect;

    const auto emitFilled = [&](const UiRect& r, const NativeUiColor& color, int z) {
        DrawCommand cmd;
        cmd.type = DrawCommandType::FilledRect;
        cmd.rect = r;
        cmd.color = color;
        cmd.zIndex = z;
        drawCommands_.push_back(std::move(cmd));
    };

    const auto emitRect = [&](const UiRect& r, const NativeUiColor& color, float thickness, int z) {
        DrawCommand cmd;
        cmd.type = DrawCommandType::Rect;
        cmd.rect = r;
        cmd.color = color;
        cmd.thickness = thickness;
        cmd.zIndex = z;
        drawCommands_.push_back(std::move(cmd));
    };

    const auto emitText = [&](std::string_view text, float x, float y, const NativeUiColor& color,
                              float scale, int z) {
        if (text.empty()) return;
        DrawCommand cmd;
        cmd.type = DrawCommandType::Text;
        cmd.text.assign(text.begin(), text.end());
        cmd.textX = x;
        cmd.textY = y;
        cmd.color = color;
        cmd.textScale = scale;
        cmd.zIndex = z;
        drawCommands_.push_back(std::move(cmd));
    };

    NativeUiColor bg = node.style.background;
    NativeUiColor border = node.style.border;
    NativeUiColor textColor = node.style.text;

    const float opacity = ClampFloat(node.style.opacity, 0.0f, 1.0f);
    bg.a *= opacity;
    border.a *= opacity;
    textColor.a *= opacity;

    if (!node.enabled) {
        bg = node.style.disabled;
        border = node.style.disabled;
        textColor = node.style.disabled;
    }

    if (node.state.hovered) {
        bg.r = ClampFloat(bg.r + 0.08f, 0.0f, 1.0f);
        bg.g = ClampFloat(bg.g + 0.08f, 0.0f, 1.0f);
        bg.b = ClampFloat(bg.b + 0.08f, 0.0f, 1.0f);
    }
    if (node.state.pressed) {
        bg = node.style.accent;
    }

    const int z = node.layout.zIndex;

    switch (node.type) {
        case WidgetType::Root:
            return;

        case WidgetType::Divider:
            emitFilled({rect.x, rect.y, rect.w, std::max(1.0f, rect.h)}, border, z);
            return;

        case WidgetType::Spacer:
            return;

        case WidgetType::Label:
        case WidgetType::Badge:
        case WidgetType::Tooltip:
        case WidgetType::InlineValidationMessage:
        case WidgetType::Notification:
        case WidgetType::Toast:
        case WidgetType::Alert:
            emitFilled(rect, bg, z);
            emitRect(rect, border, std::max(1.0f, node.style.borderThickness), z + 1);
            emitText(node.state.text.empty() ? WidgetTypeName(node.type) : node.state.text,
                     rect.x + 8.0f, rect.y + 8.0f, textColor, node.style.fontScale, z + 2);
            return;

        case WidgetType::Checkbox:
        case WidgetType::RadioButton:
        case WidgetType::ToggleSwitch: {
            emitFilled(rect, bg, z);
            emitRect(rect, border, std::max(1.0f, node.style.borderThickness), z + 1);

            UiRect marker{rect.x + 6.0f, rect.y + 6.0f, rect.h - 12.0f, rect.h - 12.0f};
            emitRect(marker, border, 1.0f, z + 2);
            if (node.state.checked) {
                emitFilled({marker.x + 3.0f, marker.y + 3.0f, marker.w - 6.0f, marker.h - 6.0f},
                           node.style.accent, z + 3);
            }

            emitText(node.state.text.empty() ? WidgetTypeName(node.type) : node.state.text,
                     marker.Right() + 8.0f, rect.y + 7.0f, textColor, node.style.fontScale, z + 4);
            return;
        }

        case WidgetType::ProgressBar: {
            emitFilled(rect, bg, z);
            emitRect(rect, border, 1.0f, z + 1);
            const float norm =
                (node.state.maxValue - node.state.minValue) > kEpsilon
                    ? (node.state.value - node.state.minValue) /
                          (node.state.maxValue - node.state.minValue)
                    : 0.0f;
            const float w = rect.w * ClampFloat(norm, 0.0f, 1.0f);
            emitFilled({rect.x, rect.y, w, rect.h}, node.style.accent, z + 2);
            return;
        }

        case WidgetType::Slider:
        case WidgetType::RangeSlider:
        case WidgetType::NumberInput:
        case WidgetType::Knob:
        case WidgetType::Rating: {
            emitFilled(rect, bg, z);
            emitRect(rect, border, 1.0f, z + 1);

            const UiRect track{rect.x + 8.0f, rect.y + rect.h * 0.5f - 2.0f, rect.w - 16.0f, 4.0f};
            emitFilled(track, border, z + 2);

            const float denom = std::max(kEpsilon, node.state.maxValue - node.state.minValue);
            const float norm = ClampFloat((node.state.value - node.state.minValue) / denom, 0.0f, 1.0f);
            const float hx = track.x + norm * track.w;
            emitFilled({hx - 5.0f, rect.y + 6.0f, 10.0f, rect.h - 12.0f}, node.style.accent, z + 3);

            if (node.type == WidgetType::RangeSlider) {
                const float norm2 =
                    ClampFloat((node.state.secondaryValue - node.state.minValue) / denom, 0.0f, 1.0f);
                const float hx2 = track.x + norm2 * track.w;
                emitFilled({hx2 - 5.0f, rect.y + 6.0f, 10.0f, rect.h - 12.0f}, node.style.accent,
                           z + 3);
            }

            if (!node.state.text.empty()) {
                emitText(node.state.text, rect.x + 8.0f, rect.y + 4.0f, textColor, 0.9f, z + 4);
            }
            return;
        }

        case WidgetType::TextInput:
        case WidgetType::SearchInput:
        case WidgetType::PasswordInput:
        case WidgetType::TextArea:
        case WidgetType::RichTextEditor:
        case WidgetType::CodeEditor:
        case WidgetType::AutoCompleteInput: {
            emitFilled(rect, bg, z);
            emitRect(rect,
                     node.state.focused ? node.style.accent : border,
                     node.state.focused ? 2.0f : 1.0f,
                     z + 1);

            std::string display = node.state.valueText;
            if (node.state.password) {
                display.assign(display.size(), '*');
            }
            if (display.empty()) {
                display = node.state.placeholder.empty() ? "..." : node.state.placeholder;
            }

            emitText(display, rect.x + 8.0f, rect.y + 8.0f, textColor, node.style.fontScale, z + 2);
            return;
        }

        default:
            break;
    }

    EmitGenericWidget(node, WidgetTypeName(node.type));
}

void RetainedUiContext::EmitGenericWidget(const UiNode& node, std::string_view fallbackLabel) {
    DrawCommand bg;
    bg.type = DrawCommandType::FilledRect;
    bg.rect = node.worldRect;
    bg.color = node.style.background;
    bg.zIndex = node.layout.zIndex;
    drawCommands_.push_back(std::move(bg));

    DrawCommand border;
    border.type = DrawCommandType::Rect;
    border.rect = node.worldRect;
    border.color = node.style.border;
    border.thickness = std::max(1.0f, node.style.borderThickness);
    border.zIndex = node.layout.zIndex + 1;
    drawCommands_.push_back(std::move(border));

    const std::string_view header =
        !node.state.title.empty() ? std::string_view(node.state.title) :
        !node.state.text.empty() ? std::string_view(node.state.text) : fallbackLabel;

    DrawCommand text;
    text.type = DrawCommandType::Text;
    text.text.assign(header.begin(), header.end());
    text.textX = node.worldRect.x + 8.0f;
    text.textY = node.worldRect.y + 8.0f;
    text.color = node.style.text;
    text.textScale = node.style.fontScale;
    text.zIndex = node.layout.zIndex + 2;
    drawCommands_.push_back(std::move(text));

    if ((node.type == WidgetType::List || node.type == WidgetType::DataGrid ||
         node.type == WidgetType::Table || node.type == WidgetType::TreeView ||
         node.type == WidgetType::VirtualList || node.type == WidgetType::DragDropList ||
         node.type == WidgetType::KanbanBoard) &&
        !node.state.items.empty()) {
        float y = node.worldRect.y + 30.0f;
        const float rowH = 18.0f;
        for (size_t i = 0; i < node.state.items.size() && i < 24; ++i) {
            if (y + rowH > node.worldRect.Bottom()) break;
            DrawCommand row;
            row.type = DrawCommandType::Text;
            row.text = node.state.items[i];
            row.textX = node.worldRect.x + 10.0f;
            row.textY = y;
            row.color = node.style.text;
            row.textScale = 0.9f;
            row.zIndex = node.layout.zIndex + 2;
            drawCommands_.push_back(std::move(row));
            y += rowH;
        }
    }
}

UiNode* RetainedUiContext::FindTopInteractiveAt(float x, float y) {
    const UiNode* hit = static_cast<const RetainedUiContext*>(this)->FindTopInteractiveAt(x, y);
    return const_cast<UiNode*>(hit);
}

const UiNode* RetainedUiContext::FindTopInteractiveAt(float x, float y) const {
    const UiNode* best = nullptr;
    int bestZ = std::numeric_limits<int>::min();

    std::function<void(const UiNode&)> visit = [&](const UiNode& node) {
        if (!node.layout.visible) return;

        for (auto it = node.children.rbegin(); it != node.children.rend(); ++it) {
            const UiNode* child = FindComponent(*it);
            if (child) visit(*child);
        }

        const bool interactive =
            node.layout.interactable || node.layout.resizable || node.state.draggable;
        if (!interactive || !node.enabled) return;
        if (!node.worldRect.Contains(x, y)) return;

        if (node.layout.zIndex >= bestZ) {
            bestZ = node.layout.zIndex;
            best = &node;
        }
    };

    const UiNode* root = FindComponent(rootId_);
    if (root) visit(*root);

    return best;
}

}  // namespace se::ui::retained
