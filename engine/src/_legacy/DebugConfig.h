#pragma once

// ============================================================================
// Debug Configuration - Compile-time switches for debug features
// ============================================================================

// SE_DEBUG detection:
// - NDEBUG is defined by C++ standard in Release builds (also by CMake)
// - DEBUG/_DEBUG are typically defined in Debug builds
// - We check NDEBUG first as it's the most reliable indicator of Release
#if defined(NDEBUG)
    // Release build - disable debug features
    #define SE_DEBUG 0
    #define SE_ENABLE_PROFILER 0
    #define SE_ENABLE_DEBUG_TOOLS 0
#elif defined(DEBUG) || defined(_DEBUG)
    // Debug build - enable debug features
    #define SE_DEBUG 1
    #define SE_ENABLE_PROFILER 1
    #define SE_ENABLE_DEBUG_TOOLS 1
#else
    // Unknown - default to release behavior for safety
    #define SE_DEBUG 0
    #define SE_ENABLE_PROFILER 0
    #define SE_ENABLE_DEBUG_TOOLS 0
#endif

// Feature flags that can be overridden per-project
#ifndef SE_PROFILER_HISTORY_FRAMES
    #define SE_PROFILER_HISTORY_FRAMES 300
#endif

#ifndef SE_PROFILER_MAX_TASKS
    #define SE_PROFILER_MAX_TASKS 100
#endif
