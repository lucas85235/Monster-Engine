---
description: Material System Refactoring - Continue from current phase
---

# Material System Refactoring Workflow

This workflow maintains context for the material system refactoring. Always follow these steps.

## Before Starting Any Work

1. Read the implementation plan to understand current progress:
   ```
   Read file: MATERIAL_REFACTORING_PLAN.md
   ```

2. Check the current phase by looking at the "Current Progress" section at the bottom of the plan.

3. Read the architecture analysis for technical context:
   ```
   Read file: C:\Users\rapha\.gemini\antigravity\brain\a5e858ae-f44d-4d92-857b-32e3f07ed02a\material_architecture_analysis.md
   ```

## Working on a Phase

4. For each phase, follow these files as reference for the current material system:

   **Core Material Types (to be replaced):**
   - `engine/include/engine/renderer/Material.h` - Base class
   - `engine/include/engine/renderer/PBRMaterial.h` - PBR wrapper + params
   - `engine/include/engine/renderer/TextureMaterial.h` - Texture container
   - `engine/include/engine/resources/MaterialManager.h` - Current manager

   **Usage Sites (to be updated):**
   - `engine/src/ecs/RenderSystem.cpp` - Main rendering loop
   - `engine/src/Renderer/SceneRenderer.cpp` - Submit methods
   - `engine/include/engine/ecs/SimpleComponents.h` - MeshRenderComponent
   - `engine/include/engine/resources/SubMesh.h` - Static mesh materials
   - `engine/include/engine/animation/SkinnedMesh.h` - Skinned mesh materials

   **Editor (to be migrated):**
   - `tools/map_editor/src/core/EditorMaterialData.h` - Editor material type
   - `tools/map_editor/src/ui/MaterialEditorPanel.h` - Material editor UI

5. When implementing, always:
   - Maintain backward compatibility until Phase 7
   - Add logging for debugging
   - Update the checklist in MATERIAL_REFACTORING_PLAN.md after completing items

## After Completing Work

6. Update the "Current Progress" section in MATERIAL_REFACTORING_PLAN.md:
   - Change the Phase name
   - Update the Last Updated date

7. Run the build to verify no regressions:
   ```
   // turbo
   scripts\gen_solutions.bat
   ```
   
   ```
   // turbo
   cmake --build build --config Release
   ```

8. Test the third_person_game to verify rendering works:
   ```
   build\apps\third_person_game\Release\third_person_game.exe
   ```

## Key Architecture Decisions

- **MaterialDefinition**: Data-only struct, UBO-aligned, serializable
- **MaterialInstance**: GPU resources, owns UBO + textures
- **MaterialLibrary**: Singleton cache, owns all instances
- **Feature flags**: Used for shader permutation selection
- **Texture slots**: Centralized (0=shadow, 1-7=PBR, 8-10=IBL)

## Common Pitfalls to Avoid

- Do NOT remove old types until Phase 7
- Do NOT change texture slot assignments without updating ALL shaders
- Do NOT forget to update both static and skinned mesh paths
- Do NOT forget editor material types are in `mst::` namespace
- Always check both `model.frag` and `skinned_model.frag` for shader changes

## Files Created by This Refactoring

**New engine files:**
- `engine/include/engine/renderer/MaterialDefinition.h`
- `engine/include/engine/renderer/MaterialInstance.h`
- `engine/include/engine/resources/MaterialLibrary.h`
- `engine/src/Renderer/MaterialDefinition.cpp`
- `engine/src/Renderer/MaterialInstance.cpp`
- `engine/src/resources/MaterialLibrary.cpp`

**Modified files:**
- Listed in each phase of MATERIAL_REFACTORING_PLAN.md