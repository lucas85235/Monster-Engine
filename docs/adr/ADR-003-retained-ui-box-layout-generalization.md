## ADR-003: Retained UI Box Layout Generalization
**Status**: Accepted

**Context**:
- The retained native UI already had stack containers (`VStack`, `HStack`), but stack behavior was limited for broad application scenarios.
- We needed explicit, reusable support for common box-layout semantics (vertical/horizontal box naming, spacing distribution, cross-axis alignment, and per-widget margins).
- A concrete runtime validation target was requested: a panel with vertically arranged `Start` and `Quit` buttons.

**Decision**:
- Add explicit box aliases to the retained API:
  - `EnsureVerticalBox(...)` (maps to `VStack`)
  - `EnsureHorizontalBox(...)` (maps to `HStack`)
- Extend `LayoutStyle` with stack controls:
  - `justifyContent` (`Start`, `Center`, `End`, `SpaceBetween`, `SpaceAround`, `SpaceEvenly`)
  - `alignItems` (`Auto`, `Start`, `Center`, `End`, `Stretch`)
  - Per-widget margins (`marginLeft`, `marginTop`, `marginRight`, `marginBottom`)
- Update stack layout resolution to apply:
  - Primary-axis growth and distribution
  - Justify behavior for remaining space
  - Cross-axis alignment with optional stretch
  - Per-widget margins in both axes
- Add `ui.retained.menu` console command to build a runtime test panel with vertical `Start` and `Quit` buttons.

**Alternatives considered**:
- Keep current stack behavior and only add a one-off menu command:
  - Pros: minimal change.
  - Cons: does not improve general-purpose layout capability.
- Introduce a separate full Flexbox engine:
  - Pros: richer feature set.
  - Cons: larger complexity and migration surface than needed now.

**Consequences**:
- Positive:
  - Better layout expressiveness for different UI domains (tools, runtime HUDs, menus, utility apps).
  - Improved API clarity with explicit vertical/horizontal box helpers.
  - Practical smoke-test entry point via `ui.retained.menu`.
- Trade-offs:
  - Additional layout knobs increase configuration surface.
  - Layout behavior is still custom retained logic (not full CSS/Flexbox parity).
