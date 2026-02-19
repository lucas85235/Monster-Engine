#include "engine/editor/panels/PerformancePanel.h"

#include <imgui.h>

#include "engine/ecs/Scene.h"

namespace se {

void PerformancePanel::OnImGuiRender() {
    ImGui::Begin(GetName(), &open_);

    ImGuiIO& io = ImGui::GetIO();

    ImGui::Text("FPS: %.1f", io.Framerate);
    ImGui::Text("Frame Time: %.3f ms", 1000.0f / io.Framerate);

    ImGui::Separator();

    if (scene_) {
        ImGui::Text("Entities: %zu", scene_->GetEntityCount());
    } else {
        ImGui::TextDisabled("No scene");
    }

    ImGui::Separator();

    // Frame time graph
    static float frameTimes[120] = {0};
    static int frameIndex = 0;
    frameTimes[frameIndex] = 1000.0f / io.Framerate;
    frameIndex = (frameIndex + 1) % 120;

    ImGui::PlotLines("##frametime", frameTimes, 120, frameIndex,
                     "Frame Time (ms)", 0.0f, 33.3f, ImVec2(0, 60));

    ImGui::End();
}

}  // namespace se
