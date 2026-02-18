#include "editor/EditorGrid.h"

#include "engine/Application.h"
#include "engine/Log.h"
#include "engine/renderer/MaterialSystem.h"
#include "engine/renderer/MeshData.h"
#include "engine/renderer/MeshSystem.h"

namespace mst {

EditorGrid::~EditorGrid() {
    if (gridHandle_.IsValid()) {
        auto& meshSystem = se::Application::Get().GetMeshSystem();
        meshSystem.DestroyRenderable(gridHandle_);
    }
}

void EditorGrid::Init() {
    auto& meshSystem = se::Application::Get().GetMeshSystem();
    auto& materialSystem = se::Application::Get().GetMaterialSystem();

    // Create a large subdivided ground plane as the editor grid reference.
    // 100x100 units with 20 subdivisions gives a visual grid-like surface.
    auto meshData = se::MeshPrimitives::CreatePlane(100.0f, 100.0f, 20);
    meshData.name = "EditorGrid";

    // Use an unlit material with a dark gray color so the grid is visible
    // regardless of lighting conditions.
    gridMaterial_ = materialSystem.GetDefaultUnlit();
    if (gridMaterial_.IsValid()) {
        gridMaterial_.SetColor(0.25f, 0.25f, 0.28f, 1.0f);
    }

    if (meshData.IsValid() && gridMaterial_.IsValid()) {
        gridHandle_ = meshSystem.CreateRenderable(meshData, gridMaterial_, false);
        SE_LOG_INFO("EditorGrid: Created ground plane (100x100, unlit)");
    } else {
        SE_LOG_WARN("EditorGrid: Failed to create grid renderable");
    }
}

}  // namespace mst
