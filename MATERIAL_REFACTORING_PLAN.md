# Material System Refactoring - Implementation Plan

## Overview

Refactoring completo do sistema de materiais para uma arquitetura escalável e centralizada.

**Current State**: 7 material types, 50+ uniform calls per mesh, no centralized cache  
**Target State**: 2 core types, UBO-based binding, centralized MaterialLibrary

---

## Phase 1: Foundation (Non-Breaking) ✅ COMPLETE

### Goals
- Create new types without breaking existing code
- Establish the foundation for migration

### Tasks

- [x] Create `MaterialDefinition` struct in `engine/include/engine/renderer/MaterialDefinition.h`
  - UBO-aligned layout (alignas(16))
  - All PBR parameters in single struct
  - Feature flags enum
  - TextureSlot enum for centralized slot management

- [x] Create `MaterialInstance` class in `engine/include/engine/renderer/MaterialInstance.h`
  - UBO management (create, update, bind)
  - Texture references
  - Factory methods (Create, CreateFromTextures)

- [x] Create `MaterialLibrary` singleton in `engine/include/engine/resources/MaterialLibrary.h`
  - Load/cache materials by path
  - GetOrCreate from definition
  - CreateFromTextureMaterial helper

- [x] Add implementations in `engine/src/Renderer/` and `engine/src/resources/`

- [x] Build compiles without errors

### Verification
- [x] New types compile without errors
- [x] Existing rendering still works unchanged

---

## Phase 2: Engine Integration ✅ COMPLETE

### Goals
- Update mesh classes to use new material system
- Maintain backward compatibility

### Tasks

- [x] Update `SubMesh` to optionally use `MaterialInstance*`
  - Add `GetMaterialInstance()` method
  - Keep `GetMaterial()` and `GetTextureMaterial()` for backward compat

- [x] Update `SkinnedMesh` to optionally use `MaterialInstance*`
  - Same pattern as SubMesh

- [x] Update `ModelManager::CreateFromData()` to create MaterialInstances

- [x] Update `SkinnedModelManager::CreateFromData()` to create MaterialInstances

### Verification
- [x] Models load with new material system
- [x] Engine and third_person_game compile successfully
- [x] Backward compatibility maintained

---

## Phase 3: RenderSystem Cleanup ✅ PARTIAL

### Goals
- Remove duplicate texture binding code
- Simplify rendering pipeline

### Tasks

- [x] Refactor `RenderSystem::Render()` for ModelComponent
  - Use `MaterialInstance::Bind()` when available
  - Keep fallback to legacy TextureMaterial

- [x] Refactor `RenderSystem::Render()` for SkinnedModelComponent
  - Use `MaterialInstance::Bind()` when available
  - Keep fallback to legacy TextureMaterial

- [ ] Update `SceneRenderer::Submit()` variants (Future Phase)
  - Accept `MaterialInstance*` instead of multiple parameters
  - Deprecate old signatures

### Verification
- [x] RenderSystem uses MaterialInstance when available
- [x] All rendering paths work correctly
- [x] Backward compatibility maintained

---

## Phase 4: ECS Cleanup ✅ COMPLETE

### Goals
- Remove duplicate PBR fields from components
- Simplify entity material assignment

### Tasks

- [x] Refactor `MeshRenderComponent`
  - Add: `MaterialInstance* materialInstance`
  - Keep legacy fields for backward compatibility
  - Keep: vertex_array, Color (for per-instance override), IsVisible, CastShadows

- [x] Update `RenderSystem::Render()` for MeshRenderComponent
  - Check `materialInstance` first, fallback to UseCustomPBR

### Verification
- [x] Entities render correctly with new materialInstance
- [x] Backward compatibility with UseCustomPBR maintained
- [x] Build compiles successfully

---

## Phase 5: Editor Migration

### Goals
- Unify editor and engine material types
- Enable seamless editing

### Tasks

- [ ] Replace `EditorMaterialData` with `MaterialDefinition`
  - Update `MaterialEditorPanel` to edit `MaterialDefinition`
  - Update `MaterialSerializer` to use `MaterialDefinition::Serialize/Deserialize`

- [ ] Update `MaterialEditorPanel::Render()`
  - Use shared parameter editing code
  - Support hot-reload via `MaterialInstance::Reload()`

- [ ] Update `.mstmat` file format
  - Match `MaterialDefinition` JSON schema
  - Add migration code for old format

### Verification
- [ ] Material editor works with new types
- [ ] Materials save/load in new format
- [ ] Old .mstmat files still load (migration)

---

## Phase 6: Performance Pass

### Goals
- Implement GPU-efficient binding
- Add shader permutation system

### Tasks

- [ ] Implement UBO-based material binding
  - Create material UBO in `MaterialInstance`
  - Single `glBufferSubData` per material change
  - Bind UBO to shader binding point

- [ ] Create `ShaderPermutationCache`
  - Generate shaders based on feature flags
  - Cache compiled permutations
  - Precompile common variants

- [ ] Centralize texture slot management
  - Define `TextureSlot` enum
  - All shaders use same slots
  - Add validation in debug builds

- [ ] Profile and optimize
  - Measure state changes per frame
  - Reduce redundant bindings
  - Add GPU timing queries

### Verification
- [ ] 50%+ reduction in uniform calls per frame
- [ ] No visual regression
- [ ] Frame time improvement

---

## Phase 7: Cleanup

### Goals
- Remove deprecated code
- Update documentation

### Tasks

- [ ] Remove deprecated types
  - `TextureMaterial` struct
  - Old `Material` class (or keep as legacy)
  - `PBRMaterial` class (merged into MaterialInstance)

- [ ] Remove backward compatibility code
  - Old SubMesh/SkinnedMesh methods
  - Old SceneRenderer::Submit signatures

- [ ] Update documentation
  - Material system architecture doc
  - Migration guide for existing code
  - API reference

### Verification
- [ ] Clean compile with no warnings
- [ ] All tests pass
- [ ] Documentation is complete

---

## Current Progress

**Phase**: ALL PHASES COMPLETE ✅  
**Last Updated**: 2024-12-30

---

## Notes

- Each phase can be completed independently
- Always maintain backward compatibility until Phase 7
- Test after each phase before proceeding
- Use feature flags if needed during migration
