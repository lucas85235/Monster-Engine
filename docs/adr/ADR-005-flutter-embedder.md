## ADR-005: Flutter Embedder API Integration (In-Process Software Rendering)

**Status**: Accepted
**Date**: 2026-02-18

## Context

Monster Engine needs a production-grade UI framework for complex, data-rich interfaces
(e.g., inspectors, profilers, node editors) beyond what the existing ImGui and NativeUI
layers provide. The engine already uses Filament for 3D rendering (Metal/Vulkan) and
GLFW for windowing.

Three approaches were evaluated:

1. **Flutter Embedder API (in-process, software rendering)** — chosen
2. Flutter Embedder API with GPU texture sharing
3. Flutter as external process with IPC

## Decision

Use the **Flutter Embedder API** in software rendering mode:

- Flutter runs in-process via `FlutterEngineRun`.
- Software renderer produces RGBA pixel buffers each frame.
- Pixel buffer is uploaded to a Filament `Texture` and composited as a fullscreen
  overlay quad (same pattern as `ImGuiRenderer` and `NativeUiRenderer`).
- Input events are forwarded from GLFW via `FlutterEngineSendPointerEvent` / `FlutterEngineSendKeyEvent`.
- Bidirectional Dart↔C++ communication uses platform channels with JSON codec.
- Build-time opt-in via `SE_ENABLE_FLUTTER=ON` CMake option (defaults to OFF).
- Flutter engine shared library is downloaded by `scripts/setup_flutter.sh` (not vendored).

### Compositing order (layer masks)

| Layer | Mask   | System  |
|-------|--------|---------|
| 5     | `0x20` | Flutter |
| 6     | `0x40` | ImGui   |
| 7     | `0x80` | NativeUI|

## Alternatives Considered

### GPU texture sharing (Metal IOSurface / Vulkan interop)
- **Pros**: Zero CPU-GPU copy overhead.
- **Cons**: Platform-specific (Metal on macOS, Vulkan on Linux), complex synchronization,
  tight coupling to graphics API. Contradicts the engine's backend-abstraction design.

### External process + IPC
- **Pros**: Full isolation, no ABI coupling.
- **Cons**: Extra latency (IPC round-trip), complex serialization, separate window
  management, no shared memory model for real-time data like ECS state.

## Consequences

### Positive
- Zero platform-specific graphics API coupling.
- Reuses proven overlay pattern (Scene/View/Camera/Texture stack).
- Built-in hot reload via Dart VM Service Protocol.
- Clean opt-in/opt-out — zero impact when disabled.
- Rich widget ecosystem immediately available.

### Negative
- CPU cost of software rendering: ~1-4ms/frame when UI changes, <0.2ms when static
  (acceptable for overlay UI on modern hardware).
- Additional ~35 MB binary size from Flutter engine shared library.
- Requires downloading the Flutter engine artifact (managed by setup script).

### Risks
- Flutter Embedder API is marked stable but evolves; pin to specific engine versions.
- Software rendering performance may degrade on very high-resolution displays (4K+);
  can be mitigated by rendering Flutter at a lower internal resolution and upscaling.
