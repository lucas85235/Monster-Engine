#include "DetailsPanel.h"
#include "../core/EditorContext.h"

#include <imgui.h>

DetailsPanel::DetailsPanel(EditorContext& context) : context_(context) {
}

DetailsPanel::~DetailsPanel() = default;

void DetailsPanel::Render() {
    ImGui::Begin("Details");

    int selectedNode = context_.GetSelectedNode();
    int selectedLink = context_.GetSelectedLink();

    if (selectedNode >= 0) {
        RenderNodeDetails();
    } else if (selectedLink >= 0) {
        RenderLinkDetails();
    } else {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Select a node or link");
    }

    ImGui::End();
}

void DetailsPanel::RenderNodeDetails() {
    int nodeId = context_.GetSelectedNode();

    for (auto& node : context_.GetNodes()) {
        if (node.id == nodeId) {
            ImGui::Text("Node ID: %d", node.id);
            ImGui::Text("Type: %d", node.nodeType);
            ImGui::Separator();

            char nameBuf[128];
            strncpy(nameBuf, node.name.c_str(), sizeof(nameBuf) - 1);
            nameBuf[sizeof(nameBuf) - 1] = '\0';

            if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
                node.name = nameBuf;
            }

            ImGui::Text("Position: (%.1f, %.1f)", node.posX, node.posY);

            ImGui::Separator();
            ImGui::Text("Input Pins: %zu", node.inputPins.size());
            ImGui::Text("Output Pins: %zu", node.outputPins.size());

            break;
        }
    }
}

void DetailsPanel::RenderLinkDetails() {
    int linkId = context_.GetSelectedLink();

    for (const auto& link : context_.GetLinks()) {
        if (link.id == linkId) {
            ImGui::Text("Link ID: %d", link.id);
            ImGui::Text("Start Pin: %d", link.startPin);
            ImGui::Text("End Pin: %d", link.endPin);
            break;
        }
    }
}
