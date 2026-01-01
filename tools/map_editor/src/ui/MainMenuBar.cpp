#include "ui/MainMenuBar.h"

#include <imgui.h>

namespace mst {

MenuBarActions MainMenuBar::Render() {
    MenuBarActions actions;

    if (ImGui::BeginMainMenuBar()) {
        RenderFileMenu(actions);
        RenderEditMenu(actions);
        RenderCreateMenu(actions);
        RenderViewMenu(actions);
        RenderToolsMenu(actions);

        ImGui::EndMainMenuBar();
    }

    return actions;
}

void MainMenuBar::RenderFileMenu(MenuBarActions& actions) {
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Map", "Ctrl+N")) {
            actions.newMap = true;
        }
        
        if (ImGui::MenuItem("Open Map...", "Ctrl+O")) {
            actions.openMap = true;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Export Map...", "Ctrl+E")) {
            actions.exportMap = true;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Exit", "Alt+F4")) {
            actions.exitApp = true;
        }

        ImGui::EndMenu();
    }
}

void MainMenuBar::RenderEditMenu(MenuBarActions& actions) {
    if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem("Delete", "Delete")) {
            actions.deleteSelected = true;
        }

        if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
            actions.duplicateSelected = true;
        }

        ImGui::EndMenu();
    }
}

void MainMenuBar::RenderCreateMenu(MenuBarActions& actions) {
    if (ImGui::BeginMenu("Create")) {
        if (ImGui::MenuItem("Cube")) {
            actions.createCube = true;
        }
        if (ImGui::MenuItem("Sphere")) {
            actions.createSphere = true;
        }
        if (ImGui::MenuItem("Capsule")) {
            actions.createCapsule = true;
        }
        if (ImGui::MenuItem("Cylinder")) {
            actions.createCylinder = true;
        }
        if (ImGui::MenuItem("Plane")) {
            actions.createPlane = true;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Player Start")) {
            actions.createPlayerStart = true;
        }

        ImGui::EndMenu();
    }
}

void MainMenuBar::RenderViewMenu(MenuBarActions& actions) {
    if (ImGui::BeginMenu("View")) {
        ImGui::Text("Panels");
        ImGui::Separator();
        
        if (panelVisibility_.viewport) {
            ImGui::MenuItem("Viewport", nullptr, panelVisibility_.viewport);
        }
        if (panelVisibility_.hierarchy) {
            ImGui::MenuItem("Hierarchy", nullptr, panelVisibility_.hierarchy);
        }
        if (panelVisibility_.properties) {
            ImGui::MenuItem("Properties", nullptr, panelVisibility_.properties);
        }
        if (panelVisibility_.statusBar) {
            ImGui::MenuItem("Status Bar", nullptr, panelVisibility_.statusBar);
        }
        
        ImGui::Separator();
        ImGui::Text("Display");
        ImGui::Separator();
        
        if (ImGui::MenuItem("Toggle Grid", "G")) {
            actions.toggleGrid = true;
        }
        if (ImGui::MenuItem("Show Colliders", "C")) {
            actions.toggleColliderDebug = true;
        }
        if (ImGui::MenuItem("Reset Camera", "F")) {
            actions.resetCamera = true;
        }

        ImGui::EndMenu();
    }
}

void MainMenuBar::RenderToolsMenu(MenuBarActions& actions) {
    if (ImGui::BeginMenu("Tools")) {
        if (ImGui::MenuItem("Material Editor", "M")) {
            actions.openMaterialEditor = true;
        }
        ImGui::EndMenu();
    }
}

}  // namespace mst

