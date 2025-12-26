#pragma once

#include <memory>

#include "core/UIEditorContext.h"
#include "ui/WidgetPalettePanel.h"
#include "ui/CanvasPanel.h"
#include "ui/PropertyEditorPanel.h"
#include "ui/HierarchyPanel.h"
#include "ui/StyleEditorPanel.h"
#include "ui/PreviewWindowPanel.h"
#include "ui/FileDialogManager.h"
#include "engine/core/Layer.h"

namespace ued {

class UIEditorLayer : public se::Layer {
public:
    UIEditorLayer();
    ~UIEditorLayer() override;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float ts) override;
    void OnRender() override;
    void OnImGuiRender() override;

private:
    void SetupDockspace();
    void MainMenuBar();
    
    se::Scope<UIEditorContext> context_;
    
    // UI Panels
    se::Scope<WidgetPalettePanel> palettePanel_;
    se::Scope<CanvasPanel> canvasPanel_;
    se::Scope<PropertyEditorPanel> propertyPanel_;
    se::Scope<HierarchyPanel> hierarchyPanel_;
    se::Scope<StyleEditorPanel> stylePanel_;
    se::Scope<PreviewWindowPanel> previewWindowPanel_;
    
    // Panel visibility
    bool showPalette_ = true;
    bool showHierarchy_ = true;
    bool showProperties_ = true;
    bool showStyles_ = true;
    bool showPreviewWindow_ = true;
    
    FileDialogManager fileDialogs_;
};

}  // namespace ued
