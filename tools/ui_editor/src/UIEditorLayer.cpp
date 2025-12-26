#include "UIEditorLayer.h"

#include <imgui.h>

#include "engine/core/Application.h"
#include "engine/core/Log.h"

namespace ued {

UIEditorLayer::UIEditorLayer() : se::Layer("UIEditorLayer") {}

UIEditorLayer::~UIEditorLayer() {}

void UIEditorLayer::OnAttach() {
    SE_LOG_INFO("UIEditorLayer attached");
    
    context_ = se::CreateScope<UIEditorContext>();
    
    palettePanel_ = se::CreateScope<WidgetPalettePanel>();
    canvasPanel_ = se::CreateScope<CanvasPanel>();
    propertyPanel_ = se::CreateScope<PropertyEditorPanel>();
    hierarchyPanel_ = se::CreateScope<HierarchyPanel>();
    stylePanel_ = se::CreateScope<StyleEditorPanel>();
    previewWindowPanel_ = se::CreateScope<PreviewWindowPanel>();
}

void UIEditorLayer::OnDetach() {
    SE_LOG_INFO("UIEditorLayer detached");
}

void UIEditorLayer::OnUpdate(float ts) {
    if (context_) {
        context_->GetDocument().Update();
    }
}


void UIEditorLayer::OnRender() {
    // Preview rendering is now handled by PreviewWindowPanel
}

void UIEditorLayer::OnImGuiRender() {
    SetupDockspace();
    
    MainMenuBar();
    
    if (palettePanel_) palettePanel_->Render(*context_);
    if (canvasPanel_) canvasPanel_->Render(*context_);
    if (propertyPanel_) propertyPanel_->Render(*context_);
    if (hierarchyPanel_) hierarchyPanel_->Render(*context_);
    if (stylePanel_) stylePanel_->Render(*context_);
    if (previewWindowPanel_ && showPreviewWindow_) previewWindowPanel_->Render(*context_);
    
    // Render file dialogs
    fileDialogs_.Render();
    
    ImGui::End(); // End dockspace
}

void UIEditorLayer::SetupDockspace() {
    ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    
    windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
    windowFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    
    ImGui::Begin("UI Editor", nullptr, windowFlags);
    ImGui::PopStyleVar(3);
    
    ImGuiID dockspaceId = ImGui::GetID("UIEditorDockspace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);
}

void UIEditorLayer::MainMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New", "Ctrl+N")) {
                context_->GetDocument().New();
            }
            if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                fileDialogs_.ShowOpenDialog([this](const FileDialogResult& result) {
                    if (result.confirmed) {
                        context_->GetDocument().Load(result.filename);
                    }
                });
            }
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                if (context_->GetDocument().GetFilePath().empty()) {
                    fileDialogs_.ShowSaveDialog([this](const FileDialogResult& result) {
                        if (result.confirmed) {
                            context_->GetDocument().SaveAs(result.filename);
                        }
                    });
                } else {
                    context_->GetDocument().Save();
                }
            }
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
                fileDialogs_.ShowSaveDialog([this](const FileDialogResult& result) {
                    if (result.confirmed) {
                        context_->GetDocument().SaveAs(result.filename);
                    }
                });
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                se::Application::Get().Close();
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                context_->GetCommandSystem().Undo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                context_->GetCommandSystem().Redo();
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Widget Palette", nullptr, &showPalette_);
            ImGui::MenuItem("Hierarchy", nullptr, &showHierarchy_);
            ImGui::MenuItem("Properties", nullptr, &showProperties_);
            ImGui::MenuItem("Style Editor", nullptr, &showStyles_);
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
}

}  // namespace ued
