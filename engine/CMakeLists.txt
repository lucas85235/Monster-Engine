set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(IMNODES_IMGUI_TARGET_NAME ImGui)

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
set(freetype_DIR ${third_party_dir}/freetype)


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

# FreeType Configuration
set(FT_DISABLE_ZLIB TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI TRUE CACHE BOOL "" FORCE)
add_subdirectory(${freetype_DIR})

# Create alias for RmlUi to find FreeType
if(TARGET freetype)
    add_library(Freetype::Freetype ALIAS freetype)
endif()

# RmlUi Configuration
set(RMLUI_NO_THIRDPARTY_CONTAINERS ON CACHE BOOL "" FORCE)
set(RMLUI_FONT_ENGINE "freetype" CACHE STRING "" FORCE)
add_subdirectory(${rmlui_DIR})

find_package(OpenGL REQUIRED)

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
)

# --- LIGAÇÃO DAS DEPENDÊNCIAS DA ENGINE ---

# Liga a sua engine às suas dependências.
# Agora voltamos a ligar explicitamente ao ALVO 'glad'.
target_link_libraries(simple_engine PUBLIC
        glad
        glfw
        OpenGL::GL
        glm
        ImGui
        spdlog
        stb
        Entt
        rmlui_core
        rmlui_debugger
)

# Adiciona definições de compilação necessárias
target_compile_definitions(simple_engine PUBLIC
        GLFW_INCLUDE_NONE
        GLM_ENABLE_EXPERIMENTAL
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

