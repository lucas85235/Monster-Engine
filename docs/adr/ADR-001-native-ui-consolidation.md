## ADR-001: Native UI Consolidation Baseline
**Status**: Accepted

**Context**:
- Native UI rendering (`NativeUiRenderer`) and retained UI state (`RetainedUiContext`) were initialized but not orchestrated as a first-class runtime layer.
- `DeveloperConsoleLayer` concentrated window layout, interaction handling, and rendering in one monolithic implementation.
- `ConsoleSystem` mixed console-domain concerns with native UI command/cvar registration, increasing coupling.
- The project needed a reusable baseline for native UI components beyond the developer console.

**Decision**:
- Add a dedicated `NativeUiLayer` to drive retained native UI in the frame loop (`PollInput`, `Tick`, `Render`) when user content exists.
- Keep retained UI input/render opt-in through a content check (`HasUserContent`) to avoid processing overhead and accidental input capture when the document only contains the root node.
- Move UI-specific console command/cvar registration to `ConsoleUiBindings` so `ConsoleSystem` focuses on console-domain behavior.
- Introduce reusable native UI primitives (`Rect`, `DrawPanel`, `DrawButton`) under `ui/native/widgets` and apply them in `DeveloperConsoleLayer`.

**Alternatives considered**:
- Keep the current monolithic console and defer architectural cleanup:
  - Pros: zero short-term refactor risk.
  - Cons: coupling and reuse problems continue to grow.
- Fully migrate the developer console to retained UI in one change:
  - Pros: stronger long-term consistency.
  - Cons: high migration risk and large regression surface in a single delivery.

**Consequences**:
- Positive:
  - Retained native UI now has a stable runtime integration path.
  - Console domain and UI domain are cleaner and easier to evolve independently.
  - New native UI components can reuse shared drawing primitives instead of duplicating patterns.
- Trade-offs:
  - Native UI architecture is still hybrid (immediate + retained) until broader migration is completed.
  - Additional modules increase file count and require consistent ownership boundaries.
