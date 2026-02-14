set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(IMNODES_IMGUI_TARGET_NAME ImGui)

# bullet
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BULLET2_MULTITHREADING ON CACHE BOOL "" FORCE)
set(BULLET2_USE_OPEN_MP_MULTITHREADING OFF CACHE BOOL "" FORCE)
set(BULLET2_USE_TBB_MULTITHREADING OFF CACHE BOOL "" FORCE)
set(BULLET2_USE_PPL_MULTITHREADING OFF CACHE BOOL "" FORCE)


# ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
# ┃                   COMPILATION RELATED                   ┃
# ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
set(CMAKE_UNITY_BUILD OFF)


# ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
# ┃                   PLATFORM DETECTION                    ┃
# ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
if (WIN32)
    set(SIMPLEENGINE_PLATFORM_DEFINE SIMPLEENGINE_PLATFORM_WINDOWS)
elseif (APPLE)
    set(SIMPLEENGINE_PLATFORM_DEFINE SIMPLEENGINE_PLATFORM_APPLE)
elseif (UNIX)
    set(SIMPLEENGINE_PLATFORM_DEFINE SIMPLEENGINE_PLATFORM_LINUX)
endif ()

# ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
# ┃                 DEPENDENCY DIRECTORY SETUP              ┃
# ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
set(third_party_dir third_party)
set(spdlog_DIR ${third_party_dir}/spdlog)
set(imgui_DIR ${third_party_dir}/imgui)
set(imnodes_DIR ${third_party_dir}/imnodes)
set(imguizmo_DIR ${third_party_dir}/ImGuizmo)
set(glfw_DIR ${third_party_dir}/glfw)
set(glad_DIR ${third_party_dir}/glad)
set(entt_DIR ${third_party_dir}/entt)
set(stb_DIR ${third_party_dir}/stb)
set(rmlui_DIR ${third_party_dir}/RmlUi)
set(bullet_DIR ${third_party_dir}/bullet)
set(assimp_DIR ${third_party_dir}/assimp)


add_library(stb STATIC
        ${stb_DIR}/stb_image.cpp
        ${stb_DIR}/stb_image.h
)
# ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
# ┃                 IMPORT EXTERNAL PROJECTS                ┃
# ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
add_subdirectory(${spdlog_DIR})
add_subdirectory(${imgui_DIR})
add_subdirectory(${imguizmo_DIR})
add_subdirectory(${imnodes_DIR})
add_subdirectory(${glfw_DIR})
add_subdirectory(${glad_DIR})
add_subdirectory(${entt_DIR})
add_subdirectory(${bullet_DIR})

# FreeType Configuration
set(freetype_DIR ${third_party_dir}/freetype)
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
add_subdirectory(${freetype_DIR})

# Create alias for RmlUI to find FreeType
add_library(Freetype::Freetype ALIAS freetype)

# RmlUi Configuration
set(RMLUI_NO_THIRDPARTY_CONTAINERS ON CACHE BOOL "" FORCE)
set(RMLUI_FONT_ENGINE "freetype" CACHE STRING "" FORCE)
add_subdirectory(${rmlui_DIR})

# Assimp Configuration
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
add_subdirectory(${assimp_DIR})

# OpenGL no longer required — Filament handles all rendering

# ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
# ┃               FILAMENT SDK CONFIGURATION                 ┃
# ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
set(FILAMENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${third_party_dir}/filament)
set(FILAMENT_INCLUDE_DIR ${FILAMENT_DIR}/include)

# Determine Filament library directory based on platform and architecture
if (APPLE)
    execute_process(
        COMMAND uname -m
        OUTPUT_VARIABLE MACHINE_ARCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (MACHINE_ARCH STREQUAL "arm64")
        set(FILAMENT_LIB_DIR ${FILAMENT_DIR}/lib/arm64)
    else()
        set(FILAMENT_LIB_DIR ${FILAMENT_DIR}/lib/x86_64)
    endif()
elseif(WIN32)
    set(FILAMENT_LIB_DIR ${FILAMENT_DIR}/lib/x86_64)
else()
    # Linux — check for x86_64 libs first, fall back to arm64 for cross-compile setups
    if (EXISTS "${FILAMENT_DIR}/lib/x86_64")
        set(FILAMENT_LIB_DIR ${FILAMENT_DIR}/lib/x86_64)
    elseif(EXISTS "${FILAMENT_DIR}/lib/arm64")
        set(FILAMENT_LIB_DIR ${FILAMENT_DIR}/lib/arm64)
        message(WARNING "Only arm64 Filament libraries found — cross-compilation or native ARM Linux assumed")
    else()
        message(FATAL_ERROR "No Filament libraries found! Download the Filament SDK for your platform and place libraries in ${FILAMENT_DIR}/lib/")
    endif()
endif()

# Verify Filament library directory exists
if (NOT EXISTS "${FILAMENT_LIB_DIR}")
    message(FATAL_ERROR "Filament library directory not found: ${FILAMENT_LIB_DIR}")
endif()
message(STATUS "Using Filament libraries from: ${FILAMENT_LIB_DIR}")

# Create IMPORTED targets for Filament libraries
set(FILAMENT_LIBS
    filament
    backend
    bluegl
    bluevk
    filabridge
    filaflat
    filamat
    filament-iblprefilter
    geometry
    ibl
    image
    shaders
    smol-v
    uberarchive
    uberzlib
    utils
    camutils
    viewer
    stb
    zstd
    # gltfio model loading
    gltfio
    gltfio_core
    dracodec
    ktxreader
    basis_transcoder
)

foreach(lib ${FILAMENT_LIBS})
    add_library(filament_${lib} STATIC IMPORTED)
    set_target_properties(filament_${lib} PROPERTIES
        IMPORTED_LOCATION ${FILAMENT_LIB_DIR}/lib${lib}.a
    )
endforeach()

file(GLOB_RECURSE PROJECT_SRCS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Exclude legacy OpenGL files from the build
list(FILTER PROJECT_SRCS EXCLUDE REGEX ".*/_legacy/.*")

# Add Objective-C++ sources on Apple platforms
if(APPLE)
    file(GLOB_RECURSE PROJECT_MM_SRCS CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm"
    )
    list(APPEND PROJECT_SRCS ${PROJECT_MM_SRCS})
endif()

add_library(simple_engine STATIC
        ${PROJECT_SRCS}
)

target_include_directories(simple_engine PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${spdlog_DIR}/include
        ${imgui_DIR}
        ${imnodes_DIR}
        ${imguizmo_DIR}
        ${glfw_DIR}/include
        ${stb_DIR}
        ${entt_DIR}
        ${rmlui_DIR}/Include
        ${bullet_DIR}/src
        ${assimp_DIR}/include
        ${CMAKE_BINARY_DIR}/engine/${assimp_DIR}/include
        ${freetype_DIR}/include
        ${third_party_dir}/legit-profiler
        # Filament SDK headers
        ${FILAMENT_INCLUDE_DIR}
)

# --- ENGINE DEPENDENCIES ---

# Filament static libraries (order matters for linker)
set(FILAMENT_LINK_LIBS)
foreach(lib ${FILAMENT_LIBS})
    list(APPEND FILAMENT_LINK_LIBS filament_${lib})
endforeach()

target_link_libraries(simple_engine PUBLIC
        mmath
        glfw
        ImGui
        ImGuizmo
        spdlog
        stb
        Entt
        freetype
        rmlui_core
        rmlui_debugger
        BulletDynamics
        BulletCollision
        LinearMath
        assimp
        # Filament libraries
        ${FILAMENT_LINK_LIBS}
)

# Platform-specific frameworks required by Filament
if(APPLE)
    target_link_libraries(simple_engine PUBLIC
        "-framework Metal"
        "-framework MetalKit"
        "-framework Cocoa"
        "-framework CoreVideo"
        "-framework IOKit"
        "-framework QuartzCore"
        "-framework CoreGraphics"
        "-framework Foundation"
    )
    # Disable PCH for .mm files (PCH was compiled as C++, not Obj-C++)
    set_source_files_properties(
        ${PROJECT_MM_SRCS}
        PROPERTIES
            SKIP_PRECOMPILE_HEADERS ON
    )
endif()

# Adiciona definições de compilação necessárias
target_compile_definitions(simple_engine PUBLIC
        GLFW_INCLUDE_NONE
        GLM_ENABLE_EXPERIMENTAL
        BT_THREADSAFE=1
)


# ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
# ┃            IMGUI CONSUMER CONFIGURATION                 ┃
# ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛

target_compile_definitions(ImGui PUBLIC
        IMGUI_IMPL_OPENGL_LOADER_GLAD
)
target_include_directories(ImGui PUBLIC
        ${glad_DIR}/include
        ${glfw_DIR}/include
)
target_link_libraries(ImGui PUBLIC
        glfw
        glad
)

# ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
# ┃            IMGUIZMO CONSUMER CONFIGURATION              ┃
# ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
target_include_directories(ImGuizmo PUBLIC
        ${imgui_DIR}
        ${glad_DIR}/include
        ${glfw_DIR}/include
)
target_link_libraries(ImGuizmo PUBLIC
        glfw
        glad
        ImGui
        ${OPENGL_LIBRARIES}
        ${CMAKE_DL_LIBS}
)

# ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
# ┃            IMNODES CONSUMER CONFIGURATION              ┃
# ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
target_include_directories(imnodes PUBLIC
        ${imgui_DIR}
        ${glad_DIR}/include
        ${glfw_DIR}/include
)
target_link_libraries(imnodes PUBLIC
        glfw
        glad
        ImGui
        ${OPENGL_LIBRARIES}
        ${CMAKE_DL_LIBS}
)

target_precompile_headers(simple_engine
        PUBLIC
        $<$<COMPILE_LANGUAGE:CXX>:se_pch.h>
)

if (MSVC)
    target_compile_options(simple_engine PUBLIC
            /MP)
endif ()

