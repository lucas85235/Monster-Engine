## ADR-002: Dear ImGui Integration as a Native Filament Overlay
**Status**: Accepted

**Context**:
The runtime migrated to a Filament-first renderer (Metal/Vulkan) and no longer owns an OpenGL swapchain.
The previous ImGui integration depended on `imgui_impl_opengl3` and a dedicated auxiliary OpenGL GLFW window.
That path no longer matches the render pipeline and prevents in-frame tooling overlays on the main render target.

**Decision**:
Implement Dear ImGui rendering as a native Filament overlay renderer, built from scratch for this engine:
- Keep Dear ImGui context/input handling inside the engine runtime.
- Build and upload ImGui draw lists into Filament `VertexBuffer` / `IndexBuffer`.
- Render ImGui through a dedicated Filament overlay `View` + `Scene` + orthographic `Camera`.
- Use a Filament material compiled at runtime via `filamat::MaterialBuilder`.
- Execute layer UI through `Layer::OnImGuiRender()` during the main frame loop.
- Keep `EnableImGui` as the explicit runtime switch in `ApplicationSpecification`.

**Alternatives considered**:
- Keep OpenGL backend in a second window:
  - Rejected due to divergent rendering path, duplicated window lifecycle, and input inconsistencies.
- Use Filament sample/viewer helper code directly:
  - Rejected to avoid external coupling and because our engine needs custom lifecycle, input, and layer integration.
- Build all editor/runtime UI exclusively with custom native retained UI:
  - Rejected because existing tooling and debug panels are already authored in ImGui.

**Consequences**:
- Positive:
  - ImGui overlays render inside the same Filament frame and platform backend.
  - Existing `OnImGuiRender` tooling panels become usable in the Filament runtime.
  - Input capture can be respected by gameplay layers through engine-level suppression.
- Trade-offs:
  - We maintain a custom ImGui backend path (input + render) inside the engine.
  - Clip handling/material behavior is now part of engine responsibility and must be validated when upgrading ImGui or Filament.
