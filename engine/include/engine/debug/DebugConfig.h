#pragma once

// ============================================================================
// Debug Configuration - Compile-time switches for debug features
// ============================================================================

// SE_DEBUG is defined in Debug builds, undefined in Release
#if defined(DEBUG) || defined(_DEBUG)
    #define SE_DEBUG 1
    #define SE_ENABLE_PROFILER 1
    #define SE_ENABLE_DEBUG_TOOLS 1
#else
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
