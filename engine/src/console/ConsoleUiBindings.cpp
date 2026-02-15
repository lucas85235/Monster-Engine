#include "engine/console/ConsoleUiBindings.h"

#include "engine/console/ConsoleSystem.h"
#include "engine/ui/native/NativeUiRenderer.h"
#include "engine/ui/native/retained/RetainedUi.h"

namespace se::console {

void RegisterUiConsoleBindings(ConsoleSystem& console) {
    console.RegisterCommand("ui.stats", "Print native UI renderer frame stats", "ui.stats",
                            [&console](const std::vector<std::string>& /*args*/) {
                                const auto& stats = ui::NativeUiRenderer::Get().GetLastFrameStats();
                                console.AddOutput("ui.stats:");
                                console.AddOutput("  draw_calls=" + std::to_string(stats.drawCallsIssued));
                                console.AddOutput("  quads_submitted=" +
                                                  std::to_string(stats.quadsSubmitted));
                                console.AddOutput("  vertices_uploaded=" +
                                                  std::to_string(stats.verticesUploaded));
                                console.AddOutput("  indices_uploaded=" +
                                                  std::to_string(stats.indicesUploaded));
                                console.AddOutput(std::string("  geometry_uploaded=") +
                                                  (stats.geometryUploaded ? "1" : "0"));
                                console.AddOutput(std::string("  geometry_reused=") +
                                                  (stats.geometryReused ? "1" : "0"));
                            });

    console.RegisterCommand("ui.invalidate", "Force native UI cache rebuild", "ui.invalidate",
                            [&console](const std::vector<std::string>& /*args*/) {
                                ui::NativeUiRenderer::Get().InvalidateRetainedGeometry();
                                console.AddOutput("ui.invalidate: ok");
                            });

    console.RegisterCommand("ui.retained.stats", "Print retained UI document stats", "ui.retained.stats",
                            [&console](const std::vector<std::string>& /*args*/) {
                                const auto& stats = ui::retained::RetainedUiContext::Get().GetStats();
                                console.AddOutput("ui.retained.stats:");
                                console.AddOutput("  nodes=" + std::to_string(stats.nodeCount));
                                console.AddOutput("  draw_commands=" +
                                                  std::to_string(stats.drawCommandCount));
                                console.AddOutput("  layout_passes=" +
                                                  std::to_string(stats.layoutPasses));
                                console.AddOutput("  paint_passes=" +
                                                  std::to_string(stats.paintPasses));
                                console.AddOutput("  dirty_frames=" +
                                                  std::to_string(stats.dirtyFrames));
                                console.AddOutput("  uploaded_frames=" +
                                                  std::to_string(stats.uploadedFrames));
                                console.AddOutput("  reused_frames=" +
                                                  std::to_string(stats.reusedFrames));
                            });

    console.RegisterCommand("ui.retained.reset", "Reset retained UI document tree", "ui.retained.reset",
                            [&console](const std::vector<std::string>& /*args*/) {
                                auto& retained = ui::retained::RetainedUiContext::Get();
                                if (!retained.IsInitialized()) {
                                    retained.Init();
                                }
                                retained.Reset();
                                console.AddOutput("ui.retained.reset: ok");
                            });

    console.RegisterCommand("ui.retained.demo", "Build a retained UI component showcase",
                            "ui.retained.demo",
                            [&console](const std::vector<std::string>& /*args*/) {
                                using namespace ui::retained;

                                auto& retained = RetainedUiContext::Get();
                                if (!retained.IsInitialized()) {
                                    retained.Init();
                                }
                                retained.Reset();

                                constexpr UiId kWindow   = 1000;
                                constexpr UiId kRow1     = 1010;
                                constexpr UiId kRow2     = 1020;
                                constexpr UiId kRow3     = 1030;
                                constexpr UiId kDataList = 1040;
                                constexpr UiId kFooter   = 1050;

                                auto& window = retained.EnsureWindow(kWindow, kInvalidId, "Retained UI Lab");
                                LayoutStyle windowLayout = window.layout;
                                windowLayout.mode        = LayoutMode::VStack;
                                windowLayout.x           = 48.0f;
                                windowLayout.y           = 48.0f;
                                windowLayout.width       = 560.0f;
                                windowLayout.height      = 640.0f;
                                windowLayout.fillX       = false;
                                windowLayout.fillY       = false;
                                windowLayout.spacing     = 8.0f;
                                windowLayout.padding     = 10.0f;
                                windowLayout.zIndex      = 900;
                                windowLayout.interactable = true;
                                windowLayout.resizable    = true;
                                retained.SetLayout(kWindow, windowLayout);

                                auto& row1 = retained.EnsureHStack(kRow1, kWindow, "Row 1");
                                LayoutStyle row1Layout = row1.layout;
                                row1Layout.mode        = LayoutMode::HStack;
                                row1Layout.height      = 42.0f;
                                row1Layout.fillX       = true;
                                row1Layout.fillY       = false;
                                row1Layout.spacing     = 8.0f;
                                retained.SetLayout(kRow1, row1Layout);

                                auto& row2 = retained.EnsureHStack(kRow2, kWindow, "Row 2");
                                LayoutStyle row2Layout = row2.layout;
                                row2Layout.mode        = LayoutMode::HStack;
                                row2Layout.height      = 42.0f;
                                row2Layout.fillX       = true;
                                row2Layout.fillY       = false;
                                row2Layout.spacing     = 8.0f;
                                retained.SetLayout(kRow2, row2Layout);

                                auto& row3 = retained.EnsureGrid(kRow3, kWindow, "Row 3");
                                LayoutStyle row3Layout = row3.layout;
                                row3Layout.mode        = LayoutMode::Grid;
                                row3Layout.gridColumns = 2;
                                row3Layout.height      = 120.0f;
                                row3Layout.fillX       = true;
                                row3Layout.fillY       = false;
                                row3Layout.spacing     = 8.0f;
                                retained.SetLayout(kRow3, row3Layout);

                                retained.EnsureSearchInput(1100, kRow1, "Search");
                                retained.SetPlaceholder(1100, "type to filter assets...");

                                retained.EnsureButton(1101, kRow1, "Apply");
                                retained.EnsureSplitButton(1102, kRow1, "Presets");
                                retained.EnsureToggleSwitch(1103, kRow1, "Realtime");
                                retained.SetChecked(1103, true);

                                retained.EnsureSlider(1200, kRow2, "Exposure");
                                retained.SetRange(1200, -5.0f, 5.0f);
                                retained.SetValue(1200, 1.2f);

                                retained.EnsureRangeSlider(1201, kRow2, "Luminance");
                                retained.SetRange(1201, 0.0f, 10.0f);
                                retained.SetValue(1201, 1.5f);
                                retained.SetSecondaryValue(1201, 7.0f);

                                retained.EnsureProgressBar(1202, kRow2, "Streaming");
                                retained.SetRange(1202, 0.0f, 1.0f);
                                retained.SetValue(1202, 0.64f);

                                retained.EnsureCheckbox(1300, kRow3, "SSAO");
                                retained.SetChecked(1300, true);
                                retained.EnsureCheckbox(1301, kRow3, "SSR");
                                retained.SetChecked(1301, true);
                                retained.EnsureCheckbox(1302, kRow3, "Bloom");
                                retained.SetChecked(1302, true);
                                retained.EnsureCheckbox(1303, kRow3, "Vignette");
                                retained.SetChecked(1303, false);

                                auto& dataList =
                                    retained.EnsureVirtualList(kDataList, kWindow, "Frame Breakdown");
                                LayoutStyle dataLayout = dataList.layout;
                                dataLayout.fillX       = true;
                                dataLayout.fillY       = true;
                                dataLayout.height      = 260.0f;
                                retained.SetLayout(kDataList, dataLayout);
                                retained.SetItems(kDataList,
                                                  {"Renderer::BeginFrame 1.12ms",
                                                   "ShadowPass 2.61ms",
                                                   "GeometryPass 3.42ms",
                                                   "PostProcess 1.33ms",
                                                   "UI Composite 0.18ms",
                                                   "GPU Frame 8.91ms"});

                                auto& footer = retained.EnsureHStack(kFooter, kWindow, "Footer");
                                LayoutStyle footerLayout = footer.layout;
                                footerLayout.mode        = LayoutMode::HStack;
                                footerLayout.height      = 38.0f;
                                footerLayout.fillX       = true;
                                footerLayout.fillY       = false;
                                footerLayout.spacing     = 8.0f;
                                retained.SetLayout(kFooter, footerLayout);

                                retained.EnsureButton(1500, kFooter, "Save Layout");
                                retained.EnsureButton(1501, kFooter, "Load Layout");
                                retained.EnsureCommandPalette(1502, kFooter, "Command Palette");

                                console.AddOutput("ui.retained.demo: created retained component showcase");
                            });
}

void RegisterUiConsoleCVars(ConsoleSystem& console) {
    const bool uiRetainDefault = ui::NativeUiRenderer::Get().IsRetainedGeometryReuseEnabled();
    const bool uiFlipVDefault  = ui::NativeUiRenderer::Get().IsFlipUvV();

    console.RegisterBoolCVar("ui_retain_cache", uiRetainDefault,
                             "Reuse retained native UI geometry between frames", true,
                             [](const ConsoleVar& var) {
                                 ui::NativeUiRenderer::Get().SetRetainedGeometryReuseEnabled(
                                     std::get<bool>(var.value));
                             });

    console.RegisterBoolCVar("ui_flip_uv_v", uiFlipVDefault,
                             "Flip native UI atlas V coordinate (debug compatibility toggle)", true,
                             [](const ConsoleVar& var) {
                                 ui::NativeUiRenderer::Get().SetFlipUvV(std::get<bool>(var.value));
                             });
}

}  // namespace se::console
