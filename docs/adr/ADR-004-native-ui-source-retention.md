## ADR-004: Native UI Retained Geometry by Source ID
**Status**: Accepted

**Context**:
- Native UI retained reuse was previously global for the whole frame.
- When multiple overlays shared the same renderer (e.g., retained UI document + developer console), one overlay could request reuse while another emitted new geometry, causing visual corruption/flicker.
- We needed retained behavior that is deterministic and scalable for multiple UI producers.

**Decision**:
- Introduce source-scoped retained rendering in `NativeUiRenderer`:
  - `BeginRetainedSource(sourceId, sourceDirty)` / `EndRetainedSource()`
  - Each source keeps its own cached geometry.
  - Frame composition merges active source caches into one GPU upload only when needed.
- Reuse policy is now based on source graph state:
  - Reuse previous GPU geometry only when no source is dirty and active source topology/order is unchanged.
  - Recompose and upload when any source changes or source set/order changes.
- Assign stable source IDs:
  - Retained UI context: `0x1001`
  - Developer console: `0x1002`

**Alternatives considered**:
- Keep global retain flag with conditional redraw hacks:
  - Pros: smaller change.
  - Cons: fragile coupling between overlays; not robust.
- Force full redraw every frame:
  - Pros: simple and correct.
  - Cons: loses retained-mode benefits and increases upload cost.

**Consequences**:
- Positive:
  - Retained semantics are now explicit per producer.
  - Multiple overlays can coexist without fighting over global reuse state.
  - Reuse still happens when the full UI graph is stable.
- Trade-offs:
  - Renderer state machine is more complex (source lifecycle + composition).
  - Requires stable source ID ownership discipline for new UI producers.
