#include "AnimGraphPanel.h"
#include "StateMachinePanel.h"
#include "BlendSpacePanel.h"
#include "../core/EditorContext.h"

#include <imgui.h>
#include <imnodes.h>
#include <engine/Log.h>
#include <filesystem>
#include <algorithm>

namespace NodeTypes {
    constexpr int Output = 0;
    constexpr int Clip = 1;
    constexpr int Blend = 2;
    constexpr int BlendSpace1D = 3;
    constexpr int BlendSpace2D = 4;
    constexpr int StateMachine = 5;
    constexpr int LayerBlend = 6;
}

static std::vector<std::string> s_availableClips;
static bool s_clipsScanned = false;

static void ScanClips() {
    if (s_clipsScanned) return;
    s_clipsScanned = true;
    s_availableClips.clear();
    s_availableClips.push_back("");
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator("assets")) {
            if (!entry.is_regular_file()) continue;
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".fbx" || ext == ".gltf" || ext == ".glb") {
                s_availableClips.push_back(entry.path().string());
            }
        }
    } catch (...) {}
}

AnimGraphPanel::AnimGraphPanel(EditorContext& context) : context_(context) {
}

AnimGraphPanel::~AnimGraphPanel() = default;

void AnimGraphPanel::OpenStateMachineEditor(int nodeId) {
    if (smPanel_) {
        smPanel_->SetNodeId(nodeId);
        smPanel_->SyncFromContext();
        smPanel_->SetActive(true);
        SE_LOG_INFO("[AnimGraph] Opening State Machine editor for node {}", nodeId);
    }
}

void AnimGraphPanel::OpenBlendSpaceEditor(int nodeId) {
    if (bsPanel_) {
        bsPanel_->SetNodeId(nodeId);
        bsPanel_->SyncFromContext();
        bsPanel_->SetActive(true);
        SE_LOG_INFO("[AnimGraph] Opening Blend Space editor for node {}", nodeId);
    }
}

void AnimGraphPanel::Render() {
    ImGui::Begin("Anim Graph");

    ImNodes::BeginNodeEditor();

    RenderNodes();
    RenderLinks();
    HandleNodeCreation();

    ImNodes::MiniMap(0.2f, ImNodesMiniMapLocation_BottomRight);
    ImNodes::EndNodeEditor();
    
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ANIMATION_CLIP")) {
            const char* clipPath = static_cast<const char*>(payload->Data);
            ImVec2 mousePos = ImGui::GetMousePos();
            
            int id = context_.CreateNode(NodeTypes::Clip, mousePos.x, mousePos.y);
            auto& nodes = context_.GetNodes();
            for (auto& n : nodes) {
                if (n.id == id) {
                    std::string filename = clipPath;
                    size_t lastSlash = filename.find_last_of("/\\");
                    if (lastSlash != std::string::npos) {
                        filename = filename.substr(lastSlash + 1);
                    }
                    size_t dotPos = filename.find_last_of('.');
                    if (dotPos != std::string::npos) {
                        filename = filename.substr(0, dotPos);
                    }
                    n.name = filename;
                    n.inputPins.clear();
                    context_.SetNodeClipPath(id, clipPath);
                    break;
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    HandleLinkCreation();
    HandleDeletion();

    int selectedNode = -1;
    if (ImNodes::NumSelectedNodes() == 1) {
        ImNodes::GetSelectedNodes(&selectedNode);
        context_.SelectNode(selectedNode);
    }

    ImGui::End();
}

void AnimGraphPanel::RenderNodes() {
    for (auto& node : context_.GetNodes()) {
        switch (node.nodeType) {
            case NodeTypes::Output:
                RenderOutputNode(node.id, node.posX, node.posY);
                break;
            case NodeTypes::Clip:
                RenderClipNode(node.id, node.name.c_str(), node.posX, node.posY);
                break;
            case NodeTypes::Blend:
                RenderBlendNode(node.id, node.posX, node.posY);
                break;
            case NodeTypes::BlendSpace2D:
                RenderBlendSpaceNode(node.id, node.posX, node.posY);
                break;
            case NodeTypes::StateMachine:
                RenderStateMachineNode(node.id, node.name.c_str(), node.posX, node.posY);
                break;
        }
    }
}

void AnimGraphPanel::RenderOutputNode(int nodeId, float posX, float posY) {
    ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(200, 80, 80, 255));
    ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, IM_COL32(220, 100, 100, 255));
    ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, IM_COL32(240, 120, 120, 255));

    ImNodes::BeginNode(nodeId);

    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted("Output Pose");
    ImNodes::EndNodeTitleBar();

    auto& nodes = context_.GetNodes();
    for (auto& node : nodes) {
        if (node.id == nodeId && !node.inputPins.empty()) {
            ImNodes::BeginInputAttribute(node.inputPins[0]);
            ImGui::TextUnformatted("Result");
            ImNodes::EndInputAttribute();
        }
    }

    ImNodes::EndNode();
    ImNodes::PopColorStyle();
    ImNodes::PopColorStyle();
    ImNodes::PopColorStyle();
}

void AnimGraphPanel::RenderClipNode(int nodeId, const char* name, float posX, float posY) {
    ScanClips();
    
    ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(80, 120, 200, 255));

    ImNodes::BeginNode(nodeId);

    ImNodes::BeginNodeTitleBar();
    ImGui::Text("Clip: %s", name[0] ? name : "None");
    ImNodes::EndNodeTitleBar();

    auto& nodes = context_.GetNodes();
    for (auto& node : nodes) {
        if (node.id == nodeId) {
            const std::string& currentClip = context_.GetNodeClipPath(nodeId);
            std::string displayName = currentClip.empty() ? "Select Clip..." : currentClip;
            size_t slash = displayName.find_last_of("/\\");
            if (slash != std::string::npos) displayName = displayName.substr(slash + 1);
            
            ImGui::PushItemWidth(120.0f);
            if (ImGui::BeginCombo("##Clip", displayName.c_str())) {
                for (const auto& clip : s_availableClips) {
                    std::string label = clip.empty() ? "None" : clip;
                    size_t s = label.find_last_of("/\\");
                    if (s != std::string::npos) label = label.substr(s + 1);
                    
                    bool selected = (clip == currentClip);
                    if (ImGui::Selectable(label.c_str(), selected)) {
                        context_.SetNodeClipPath(nodeId, clip);
                        
                        std::string nodeName = clip;
                        size_t ns = nodeName.find_last_of("/\\");
                        if (ns != std::string::npos) nodeName = nodeName.substr(ns + 1);
                        size_t dot = nodeName.find_last_of('.');
                        if (dot != std::string::npos) nodeName = nodeName.substr(0, dot);
                        node.name = nodeName.empty() ? "None" : nodeName;
                        
                        context_.MarkGraphDirty();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
            
            float speed = context_.GetNodePlaybackSpeed(nodeId);
            ImGui::PushItemWidth(80.0f);
            if (ImGui::DragFloat("Speed", &speed, 0.01f, 0.0f, 5.0f)) {
                context_.SetNodePlaybackSpeed(nodeId, speed);
            }
            ImGui::PopItemWidth();

            if (!node.outputPins.empty()) {
                ImNodes::BeginOutputAttribute(node.outputPins[0]);
                ImGui::Indent(60.0f);
                ImGui::TextUnformatted("Pose");
                ImNodes::EndOutputAttribute();
            }
        }
    }

    ImNodes::EndNode();
    ImNodes::PopColorStyle();
}

void AnimGraphPanel::RenderBlendNode(int nodeId, float posX, float posY) {
    ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(100, 180, 100, 255));

    ImNodes::BeginNode(nodeId);

    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted("Blend");
    ImNodes::EndNodeTitleBar();

    auto& nodes = context_.GetNodes();
    for (auto& node : nodes) {
        if (node.id == nodeId) {
            if (node.inputPins.size() >= 2) {
                ImNodes::BeginInputAttribute(node.inputPins[0]);
                ImGui::TextUnformatted("A");
                ImNodes::EndInputAttribute();

                ImNodes::BeginInputAttribute(node.inputPins[1]);
                ImGui::TextUnformatted("B");
                ImNodes::EndInputAttribute();
            }

            float alpha = context_.GetNodeBlendAlpha(nodeId);
            ImGui::PushItemWidth(80.0f);
            if (ImGui::DragFloat("Alpha", &alpha, 0.01f, 0.0f, 1.0f)) {
                context_.SetNodeBlendAlpha(nodeId, alpha);
            }
            ImGui::PopItemWidth();
            
            const std::string& param = context_.GetNodeBlendParameter(nodeId);
            char paramBuf[64];
            strncpy(paramBuf, param.c_str(), sizeof(paramBuf) - 1);
            paramBuf[sizeof(paramBuf) - 1] = '\0';
            ImGui::PushItemWidth(80.0f);
            if (ImGui::InputText("Param", paramBuf, sizeof(paramBuf))) {
                context_.SetNodeBlendParameter(nodeId, paramBuf);
            }
            ImGui::PopItemWidth();

            if (!node.outputPins.empty()) {
                ImNodes::BeginOutputAttribute(node.outputPins[0]);
                ImGui::Indent(60.0f);
                ImGui::TextUnformatted("Pose");
                ImNodes::EndOutputAttribute();
            }
        }
    }

    ImNodes::EndNode();
    ImNodes::PopColorStyle();
}

void AnimGraphPanel::RenderBlendSpaceNode(int nodeId, float posX, float posY) {
    ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(180, 140, 80, 255));

    ImNodes::BeginNode(nodeId);

    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted("BlendSpace 2D");
    ImNodes::EndNodeTitleBar();

    auto& nodes = context_.GetNodes();
    for (auto& node : nodes) {
        if (node.id == nodeId) {
            if (ImGui::Button("Edit##BS")) {
                OpenBlendSpaceEditor(nodeId);
            }
            
            const auto* bsData = context_.GetBlendSpaceData(nodeId);
            if (bsData) {
                ImGui::Text("Samples: %zu", bsData->samples.size());
            } else {
                ImGui::TextDisabled("No samples");
            }

            if (!node.outputPins.empty()) {
                ImNodes::BeginOutputAttribute(node.outputPins[0]);
                ImGui::Indent(80.0f);
                ImGui::TextUnformatted("Pose");
                ImNodes::EndOutputAttribute();
            }
        }
    }

    ImNodes::EndNode();
    ImNodes::PopColorStyle();
}

void AnimGraphPanel::RenderStateMachineNode(int nodeId, const char* name, float posX, float posY) {
    ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(140, 100, 180, 255));

    ImNodes::BeginNode(nodeId);

    ImNodes::BeginNodeTitleBar();
    ImGui::Text("State Machine: %s", name[0] ? name : "Unnamed");
    ImNodes::EndNodeTitleBar();

    if (ImGui::Button("Edit##SM")) {
        OpenStateMachineEditor(nodeId);
    }

    auto& nodes = context_.GetNodes();
    for (auto& node : nodes) {
        if (node.id == nodeId) {
            const auto* smData = context_.GetStateMachineData(nodeId);
            if (smData) {
                ImGui::Text("States: %zu", smData->states.size());
            } else {
                ImGui::TextDisabled("No states");
            }
            
            if (!node.outputPins.empty()) {
                ImNodes::BeginOutputAttribute(node.outputPins[0]);
                ImGui::Indent(80.0f);
                ImGui::TextUnformatted("Pose");
                ImNodes::EndOutputAttribute();
            }
        }
    }

    ImNodes::EndNode();
    ImNodes::PopColorStyle();
}

void AnimGraphPanel::RenderLinks() {
    for (const auto& link : context_.GetLinks()) {
        ImNodes::Link(link.id, link.startPin, link.endPin);
    }
}

void AnimGraphPanel::HandleNodeCreation() {
    const bool openPopup = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
                           ImNodes::IsEditorHovered() &&
                           ImGui::IsMouseClicked(ImGuiMouseButton_Right);

    if (openPopup) {
        ImGui::OpenPopup("AddNodePopup");
    }

    if (ImGui::BeginPopup("AddNodePopup")) {
        ImVec2 clickPos = ImGui::GetMousePosOnOpeningCurrentPopup();

        if (ImGui::MenuItem("Animation Clip")) {
            int id = context_.CreateNode(NodeTypes::Clip, clickPos.x, clickPos.y);
            auto& nodes = context_.GetNodes();
            for (auto& n : nodes) {
                if (n.id == id) {
                    n.name = "New Clip";
                    n.inputPins.clear();
                    break;
                }
            }
        }
        if (ImGui::MenuItem("Blend")) {
            int id = context_.CreateNode(NodeTypes::Blend, clickPos.x, clickPos.y);
            auto& nodes = context_.GetNodes();
            for (auto& n : nodes) {
                if (n.id == id) {
                    n.inputPins.push_back(context_.GetNodes().back().inputPins[0] + 1);
                    break;
                }
            }
        }
        if (ImGui::MenuItem("BlendSpace 2D")) {
            int id = context_.CreateNode(NodeTypes::BlendSpace2D, clickPos.x, clickPos.y);
            auto& nodes = context_.GetNodes();
            for (auto& n : nodes) {
                if (n.id == id) {
                    n.name = "BlendSpace";
                    n.inputPins.clear();
                    break;
                }
            }
        }
        if (ImGui::MenuItem("State Machine")) {
            int id = context_.CreateNode(NodeTypes::StateMachine, clickPos.x, clickPos.y);
            auto& nodes = context_.GetNodes();
            for (auto& n : nodes) {
                if (n.id == id) {
                    n.name = "Locomotion";
                    n.inputPins.clear();
                    break;
                }
            }
        }

        ImGui::EndPopup();
    }
}

void AnimGraphPanel::HandleLinkCreation() {
    int startPin, endPin;
    if (ImNodes::IsLinkCreated(&startPin, &endPin)) {
        context_.CreateLink(startPin, endPin);
    }
}

void AnimGraphPanel::HandleDeletion() {
    if (ImGui::IsKeyReleased(ImGuiKey_Delete) || ImGui::IsKeyReleased(ImGuiKey_X)) {
        int numSelectedNodes = ImNodes::NumSelectedNodes();
        if (numSelectedNodes > 0) {
            std::vector<int> selectedNodes(numSelectedNodes);
            ImNodes::GetSelectedNodes(selectedNodes.data());
            for (int nodeId : selectedNodes) {
                context_.DeleteNode(nodeId);
            }
        }

        int numSelectedLinks = ImNodes::NumSelectedLinks();
        if (numSelectedLinks > 0) {
            std::vector<int> selectedLinks(numSelectedLinks);
            ImNodes::GetSelectedLinks(selectedLinks.data());
            for (int linkId : selectedLinks) {
                context_.DeleteLink(linkId);
            }
        }
    }
}
