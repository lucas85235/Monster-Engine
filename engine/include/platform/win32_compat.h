#ifndef MONSTER_ENGINE_WIN32_COMPAT_H
#define MONSTER_ENGINE_WIN32_COMPAT_H

/**
 * Windows compatibility header (force-included via /FI on MSVC).
 *
 * Resolves macro conflicts between Windows SDK headers and Filament:
 *   - wingdi.h defines OPAQUE as 2  -> conflicts with BlendMode::OPAQUE
 *   - wingdi.h defines TRANSPARENT  -> conflicts with enum values
 *   - windef.h defines NEAR / FAR   -> conflicts with camera enums
 *   - windows.h defines min / max   -> conflicts with std::min/max
 *
 * Strategy: Define guards BEFORE windows.h is included (via GLFW) so
 * the problematic sub-headers are never pulled in. Our engine code does
 * not call GDI functions directly; GLFW compiles as a separate target
 * and is not affected by these defines.
 */

#ifdef _WIN32

// Prevent min/max macros from windows.h
#ifndef NOMINMAX
#define NOMINMAX
#endif

// Reduce windows.h bloat
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Exclude wingdi.h entirely — it defines OPAQUE, TRANSPARENT, etc.
// Safe because our engine does not use GDI functions directly.
// GLFW links against GDI but compiles as a separate CMake target.
#ifndef NOGDI
#define NOGDI
#endif

// Additional cleanup for macros that may leak from other Windows headers
#ifdef NEAR
#undef NEAR
#endif
#ifdef FAR
#undef FAR
#endif
#ifdef OPAQUE
#undef OPAQUE
#endif
#ifdef TRANSPARENT
#undef TRANSPARENT
#endif
#ifdef ERROR
#undef ERROR
#endif

#endif // _WIN32

#endif // MONSTER_ENGINE_WIN32_COMPAT_H
