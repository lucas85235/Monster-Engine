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
set(glm_DIR ${third_party_dir}/glm)
set(entt_DIR ${third_party_dir}/entt)
set(stb_DIR ${third_party_dir}/stb)
set(rmlui_DIR ${third_party_dir}/RmlUi)
set(bullet_DIR ${third_party_dir}/bullet)


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
add_subdirectory(${glm_DIR})
add_subdirectory(${entt_DIR})
add_subdirectory(${bullet_DIR})



# RmlUi Configuration
set(RMLUI_NO_THIRDPARTY_CONTAINERS ON CACHE BOOL "" FORCE)
set(RMLUI_FONT_ENGINE "none" CACHE STRING "" FORCE)
add_subdirectory(${rmlui_DIR})

find_package(OpenGL REQUIRED)
find_package(Vulkan REQUIRED)

# SPIRV-Cross (Optional - for cross-compiling SPIR-V to GLSL)
option(USE_SPIRV_CROSS "Enable SPIRV-Cross for shader cross-compilation" ON)
if(USE_SPIRV_CROSS)
    find_package(spirv_cross_core QUIET)
    find_package(spirv_cross_glsl QUIET)
    if(spirv_cross_core_FOUND AND spirv_cross_glsl_FOUND)
        message(STATUS "SPIRV-Cross found - shader cross-compilation enabled")
        set(HAS_SPIRV_CROSS TRUE)
    else()
        # Try pkg-config fallback
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(SPIRV_CROSS spirv-cross-c-shared)
        if(SPIRV_CROSS_FOUND)
            message(STATUS "SPIRV-Cross found via pkg-config")
            set(HAS_SPIRV_CROSS TRUE)
        else()
            message(WARNING "SPIRV-Cross not found. Install libspirv-cross-c-shared-dev or disable USE_SPIRV_CROSS")
            set(USE_SPIRV_CROSS OFF)
        endif()
    endif()
endif()

file(GLOB_RECURSE PROJECT_SRCS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(simple_engine STATIC
        ${PROJECT_SRCS}
)

target_include_directories(simple_engine PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${spdlog_DIR}/include
        ${imgui_DIR}
        ${imnodes_DIR}
        ${imguizmo_DIR}
        ${glad_DIR}/include
        ${glfw_DIR}/include
        ${glm_DIR}
        ${stb_DIR}
        ${entt_DIR}
        ${rmlui_DIR}/Include
        ${bullet_DIR}/src
)

# --- LIGAÇÃO DAS DEPENDÊNCIAS DA ENGINE ---

# Liga a sua engine às suas dependências.
# Agora voltamos a ligar explicitamente ao ALVO 'glad'.
target_link_libraries(simple_engine PUBLIC
        glad
        glfw
        OpenGL::GL
        Vulkan::Vulkan
        glm
        ImGui
        spdlog
        stb
        Entt
        rmlui_core
        rmlui_debugger
        BulletDynamics
        BulletCollision
        LinearMath
)

# Adiciona definições de compilação necessárias
target_compile_definitions(simple_engine PUBLIC
        GLFW_INCLUDE_NONE
        GLFW_INCLUDE_VULKAN
        GLM_ENABLE_EXPERIMENTAL
        BT_THREADSAFE=1
)

if(HAS_SPIRV_CROSS)
    target_compile_definitions(simple_engine PUBLIC HAS_SPIRV_CROSS=1)
    if(spirv_cross_core_FOUND AND spirv_cross_glsl_FOUND)
        target_link_libraries(simple_engine PUBLIC
            spirv-cross-core
            spirv-cross-glsl
        )
    elseif(SPIRV_CROSS_FOUND)
        target_include_directories(simple_engine PUBLIC ${SPIRV_CROSS_INCLUDE_DIRS})
        target_link_libraries(simple_engine PUBLIC ${SPIRV_CROSS_LIBRARIES})
    endif()
endif()


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

