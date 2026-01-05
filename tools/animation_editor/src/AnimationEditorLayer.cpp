#include "AnimationEditorLayer.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <imnodes.h>
#include <engine/Application.h>
#include <engine/Log.h>

#include "core/EditorContext.h"
#include "panels/AnimGraphPanel.h"
#include "panels/PreviewPanel.h"
#include "panels/DetailsPanel.h"
#include "panels/ParametersPanel.h"
#include "panels/AssetBrowserPanel.h"
#include "panels/StateMachinePanel.h"
#include "panels/BlendSpacePanel.h"

AnimationEditorLayer::AnimationEditorLayer()
    : Layer("AnimationEditorLayer") {
}

AnimationEditorLayer::~AnimationEditorLayer() = default;

void AnimationEditorLayer::OnAttach() {
    SE_LOG_INFO("Animation Editor attached");

    ImNodes::CreateContext();
    ImNodes::StyleColorsDark();

    ImNodesIO& io = ImNodes::GetIO();
    io.LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;

    context_ = std::make_unique<EditorContext>();
    graphPanel_ = std::make_unique<AnimGraphPanel>(*context_);
    previewPanel_ = std::make_unique<PreviewPanel>(*context_);
    detailsPanel_ = std::make_unique<DetailsPanel>(*context_);
    parametersPanel_ = std::make_unique<ParametersPanel>(*context_);
    assetBrowserPanel_ = std::make_unique<AssetBrowserPanel>(*context_);
    stateMachinePanel_ = std::make_unique<StateMachinePanel>(*context_);
    blendSpacePanel_ = std::make_unique<BlendSpacePanel>(*context_);

    graphPanel_->SetStateMachinePanel(stateMachinePanel_.get());
    graphPanel_->SetBlendSpacePanel(blendSpacePanel_.get());

    NewGraph();
}

void AnimationEditorLayer::OnDetach() {
    ImNodes::DestroyContext();
    SE_LOG_INFO("Animation Editor detached");
}

void AnimationEditorLayer::OnUpdate(float dt) {
    context_->Update(dt);
}

void AnimationEditorLayer::OnRender() {
}

void AnimationEditorLayer::OnImGuiRender() {
    SetupDockspace();

    if (showDemoWindow_) {
        ImGui::ShowDemoWindow(&showDemoWindow_);
    }

    graphPanel_->Render();
    previewPanel_->Render();
    detailsPanel_->Render();
    parametersPanel_->Render();
    assetBrowserPanel_->Render();
    stateMachinePanel_->Render();
    blendSpacePanel_->Render();
    
    RenderFileDialogs();
}

void AnimationEditorLayer::SetupDockspace() {
    static bool dockspaceOpen = true;
    static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;

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

    ImGui::Begin("DockSpace", &dockspaceOpen, windowFlags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspaceId = ImGui::GetID("AnimEditorDockspace");
    
    static bool firstTime = true;
    if (firstTime) {
        firstTime = false;
        
        ImGui::DockBuilderRemoveNode(dockspaceId);
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->Size);
        
        ImGuiID dockLeft, dockRight, dockCenter;
        ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.2f, &dockLeft, &dockCenter);
        ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Right, 0.25f, &dockRight, &dockCenter);
        
        ImGuiID dockLeftTop, dockLeftBottom;
        ImGui::DockBuilderSplitNode(dockLeft, ImGuiDir_Down, 0.5f, &dockLeftBottom, &dockLeftTop);
        
        ImGuiID dockCenterTop, dockCenterBottom;
        ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Down, 0.35f, &dockCenterBottom, &dockCenterTop);
        
        ImGuiID dockRightTop, dockRightBottom;
        ImGui::DockBuilderSplitNode(dockRight, ImGuiDir_Down, 0.4f, &dockRightBottom, &dockRightTop);
        
        ImGui::DockBuilderDockWindow("Asset Browser", dockLeftTop);
        ImGui::DockBuilderDockWindow("Parameters", dockLeftBottom);
        ImGui::DockBuilderDockWindow("Anim Graph", dockCenterTop);
        ImGui::DockBuilderDockWindow("Preview", dockCenterBottom);
        ImGui::DockBuilderDockWindow("Details", dockRightTop);
        ImGui::DockBuilderDockWindow("State Machine Editor", dockRightBottom);
        ImGui::DockBuilderDockWindow("Blend Space Editor", dockCenterBottom);
        
        ImGui::DockBuilderFinish(dockspaceId);
    }
    
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);

    RenderMenuBar();

    ImGui::End();
}

void AnimationEditorLayer::RenderMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New", "Ctrl+N")) {
                NewGraph();
            }
            if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                OpenGraph();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                SaveGraph();
            }
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
                SaveGraphAs();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                se::Application::Get().Close();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                context_->Undo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                context_->Redo();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("ImGui Demo", nullptr, &showDemoWindow_);
            ImGui::Separator();
            if (ImGui::MenuItem("State Machine Editor")) {
                stateMachinePanel_->SetActive(true);
            }
            if (ImGui::MenuItem("Blend Space Editor")) {
                blendSpacePanel_->SetActive(true);
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void AnimationEditorLayer::NewGraph() {
    context_->CreateNewGraph();
    currentFilePath_.clear();
    SE_LOG_INFO("New animation graph created");
}

void AnimationEditorLayer::OpenGraph() {
    showOpenDialog_ = true;
}

void AnimationEditorLayer::SaveGraph() {
    if (currentFilePath_.empty()) {
        SaveGraphAs();
        return;
    }
    if (context_->SaveToFile(currentFilePath_)) {
        SE_LOG_INFO("Graph saved to: {}", currentFilePath_);
    }
}

void AnimationEditorLayer::SaveGraphAs() {
    showSaveDialog_ = true;
}

void AnimationEditorLayer::RenderFileDialogs() {
    if (showOpenDialog_) {
        ImGui::OpenPopup("Open Graph");
        showOpenDialog_ = false;
    }
    
    if (ImGui::BeginPopupModal("Open Graph", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char pathBuf[256] = "graph.animgraph";
        ImGui::Text("Enter file path:");
        ImGui::InputText("##OpenPath", pathBuf, sizeof(pathBuf));
        
        ImGui::Separator();
        
        if (ImGui::Button("Open", ImVec2(100, 0))) {
            if (context_->LoadFromFile(pathBuf)) {
                currentFilePath_ = pathBuf;
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    
    if (showSaveDialog_) {
        ImGui::OpenPopup("Save Graph As");
        showSaveDialog_ = false;
    }
    
    if (ImGui::BeginPopupModal("Save Graph As", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char pathBuf[256] = "graph.animgraph";
        ImGui::Text("Enter file path:");
        ImGui::InputText("##SavePath", pathBuf, sizeof(pathBuf));
        
        ImGui::Separator();
        
        if (ImGui::Button("Save", ImVec2(100, 0))) {
            if (context_->SaveToFile(pathBuf)) {
                currentFilePath_ = pathBuf;
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}
