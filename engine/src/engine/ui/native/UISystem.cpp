#include "engine/ui/native/UISystem.h"
#include "engine/ui/native/font/UIFontManager.h"
#include "engine/Log.h"

namespace se::ui {

namespace {
    UIControl::Ptr rootControl_;
    glm::vec2 viewportSize_{1920.0f, 1080.0f};
    UIControl* focusedControl_ = nullptr;
}

void Initialize(float viewportWidth, float viewportHeight) {
    viewportSize_ = {viewportWidth, viewportHeight};
    
    // Initialize font manager FIRST
    auto& fontMgr = UIFontManager::Get();
    if (!fontMgr.Initialize()) {
        SE_LOG_ERROR("[UISystem] Failed to initialize font manager!");
    }
    
    // Initialize theme database
    auto& themeDB = UIThemeDB::Get();
    
    // Create a default theme with basic styling
    auto defaultTheme = std::make_shared<UITheme>();
    
    // Set default button styles
    auto buttonNormal = std::make_shared<UIStyleBoxFlat>();
    buttonNormal->SetBackgroundColor({0.3f, 0.3f, 0.35f, 1.0f});
    buttonNormal->SetBorderColor({0.5f, 0.5f, 0.55f, 1.0f});
    buttonNormal->SetBorderWidthAll(1.0f);
    buttonNormal->SetCornerRadiusAll(4.0f);
    buttonNormal->SetContentMarginAll(8.0f);
    defaultTheme->SetStyleBox("Button", "normal", buttonNormal);
    
    auto buttonHover = std::make_shared<UIStyleBoxFlat>();
    buttonHover->SetBackgroundColor({0.4f, 0.4f, 0.45f, 1.0f});
    buttonHover->SetBorderColor({0.6f, 0.6f, 0.65f, 1.0f});
    buttonHover->SetBorderWidthAll(1.0f);
    buttonHover->SetCornerRadiusAll(4.0f);
    buttonHover->SetContentMarginAll(8.0f);
    defaultTheme->SetStyleBox("Button", "hover", buttonHover);
    
    auto buttonPressed = std::make_shared<UIStyleBoxFlat>();
    buttonPressed->SetBackgroundColor({0.2f, 0.4f, 0.6f, 1.0f});
    buttonPressed->SetBorderColor({0.3f, 0.5f, 0.7f, 1.0f});
    buttonPressed->SetBorderWidthAll(1.0f);
    buttonPressed->SetCornerRadiusAll(4.0f);
    buttonPressed->SetContentMarginAll(8.0f);
    defaultTheme->SetStyleBox("Button", "pressed", buttonPressed);
    
    // Set default panel style
    auto panelStyle = std::make_shared<UIStyleBoxFlat>();
    panelStyle->SetBackgroundColor({0.15f, 0.15f, 0.18f, 1.0f});
    panelStyle->SetBorderColor({0.25f, 0.25f, 0.28f, 1.0f});
    panelStyle->SetBorderWidthAll(1.0f);
    defaultTheme->SetStyleBox("Panel", "panel", panelStyle);
    
    // Set default colors
    defaultTheme->SetColor("Label", "font_color", {0.9f, 0.9f, 0.9f, 1.0f});
    defaultTheme->SetColor("Button", "font_color", {1.0f, 1.0f, 1.0f, 1.0f});
    
    // Set default constants
    defaultTheme->SetConstant("BoxContainer", "separation", 4);
    
    themeDB.SetDefaultTheme(defaultTheme);
    
    SE_LOG_INFO("Native UI System initialized ({}x{})", viewportWidth, viewportHeight);
}

void Shutdown() {
    rootControl_.reset();
    focusedControl_ = nullptr;
    
    // Shutdown font manager
    UIFontManager::Get().Shutdown();
    
    SE_LOG_INFO("Native UI System shutdown");
}

void Update() {
    if (!rootControl_) return;
    
    // Process deferred layout updates
    // Walk through all containers and sort if pending
    std::function<void(UIControl*)> processContainer = [&](UIControl* control) {
        if (auto* container = dynamic_cast<UIContainer*>(control)) {
            if (container->IsSortPending()) {
                container->SortChildren();
            }
        }
        
        for (const auto& child : control->GetChildren()) {
            processContainer(child.get());
        }
    };
    
    processContainer(rootControl_.get());
}

void Render() {
    if (!rootControl_) return;
    
    // Walk tree and render visible controls
    std::function<void(UIControl*)> renderControl = [&](UIControl* control) {
        if (!control->IsVisible()) return;
        
        control->Draw();
        control->ClearRedrawFlag();
        
        for (const auto& child : control->GetChildren()) {
            renderControl(child.get());
        }
    };
    
    renderControl(rootControl_.get());
}

void SetRoot(UIControl::Ptr root) {
    rootControl_ = std::move(root);
    
    if (rootControl_) {
        // Set root to fill viewport
        rootControl_->SetAnchorsPreset(LayoutPreset::FULL_RECT);
        rootControl_->SetSize(viewportSize_);
    }
}

UIControl* GetRoot() {
    return rootControl_.get();
}

bool ProcessInput(const InputEvent& inputEvent) {
    if (!rootControl_) return false;
    
    // For mouse events, find the control under the cursor
    if (inputEvent.IsMouseMotion() || inputEvent.IsMouseButton()) {
        // Transform mouse position to local space and test
        std::function<UIControl*(UIControl*, const glm::vec2&)> findControl = 
            [&](UIControl* control, const glm::vec2& pos) -> UIControl* {
            
            if (!control->IsVisible()) return nullptr;
            if (control->GetMouseFilter() == MouseFilter::MOUSE_IGNORE) return nullptr;
            
            // Transform to local space
            glm::vec2 localPos = pos - control->GetPosition();
            
            // Check children first (front to back, last child is on top)
            const auto& children = control->GetChildren();
            for (auto it = children.rbegin(); it != children.rend(); ++it) {
                if (UIControl* found = findControl(it->get(), localPos)) {
                    return found;
                }
            }
            
            // Check self
            if (control->HasPoint(localPos)) {
                return control;
            }
            
            return nullptr;
        };
        
        UIControl* target = findControl(rootControl_.get(), inputEvent.mousePosition);
        
        if (target) {
            target->OnInput(inputEvent);
            
            if (target->GetMouseFilter() == MouseFilter::MOUSE_STOP) {
                inputEvent.Accept();
            }
        }
    }
    
    // For keyboard events, send to focused control
    if (inputEvent.IsKey() && focusedControl_) {
        focusedControl_->OnInput(inputEvent);
    }
    
    return inputEvent.IsConsumed();
}

}  // namespace se::ui
