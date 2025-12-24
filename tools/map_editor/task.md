# Map Editor System Implementation

## Overview
Create a robust map editor system inspired by Valve's Hammer editor, with:
- Scene serialization to `.mstmap` format
- Collision data support for physics integration
- ImGui-based editor interface with ImGuizmo gizmos
- Cross-platform support (Windows/Linux)

---

## Task Checklist

### Phase 1: Core Architecture
- [/] Design and implement map file format (`.mstmap`)
- [/] Create `MapEntity` data structure for serialized entities
- [/] Create `MapData` container for full map serialization
- [ ] Implement `MapSerializer` class for save/load

### Phase 2: Editor Core
- [ ] Create `MapEditorApp` as standalone application
- [ ] Create `MapEditorLayer` extending `se::Layer`
- [ ] Create `EditorScene` for managing edited entities
- [ ] Implement `EditorCamera` for orbit/pan/zoom navigation

### Phase 3: Primitive System
- [ ] Create `PrimitiveFactory` for creating editor primitives
- [ ] Implement primitive types: Cube, Sphere, Capsule, Cylinder, Plane
- [ ] Create `EditorEntity` wrapper with editor-specific metadata

### Phase 4: Selection & Gizmos
- [ ] Implement entity selection system (click/box select)
- [ ] Integrate ImGuizmo for translate/rotate/scale
- [ ] Create outline/highlight for selected entities

### Phase 5: UI Panels
- [ ] Create main menu bar (File, Edit, View, Create)
- [ ] Create entity hierarchy panel
- [ ] Create properties inspector panel
- [ ] Create collider properties panel
- [ ] Implement context menu (right-click)

### Phase 6: Operations
- [ ] Implement copy/paste/duplicate entities
- [ ] Implement delete entities
- [ ] Implement undo/redo system (optional for v1)

### Phase 7: Export & Integration
- [ ] Implement export dialog with file name selection
- [ ] Create `.mstmap` file writer
- [ ] Document format for future engine loader

### Phase 8: Verification
- [ ] Test primitive creation and manipulation
- [ ] Test export functionality
- [ ] Verify cross-platform compatibility
