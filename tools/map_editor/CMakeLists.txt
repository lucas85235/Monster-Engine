cmake_minimum_required(VERSION 3.16)

project(map_editor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

add_executable(map_editor ${SOURCES})

target_include_directories(map_editor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET map_editor PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

target_link_libraries(map_editor PRIVATE
    simple_engine
    ImGui
)

# Copy assets from both global assets and local map_editor assets
set(GLOBAL_ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")
set(LOCAL_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")

file(GLOB_RECURSE SHADER_FILES
    "${GLOBAL_ASSETS_DIR}/shaders/*.vert"
    "${GLOBAL_ASSETS_DIR}/shaders/*.frag"
    "${LOCAL_ASSETS_DIR}/shaders/*.vert"
    "${LOCAL_ASSETS_DIR}/shaders/*.frag"
)

add_custom_target(copy_map_editor_assets ALL DEPENDS ${SHADER_FILES})

add_custom_command(
    TARGET copy_map_editor_assets
    COMMAND ${CMAKE_COMMAND} -E echo "Copying assets to map_editor binary dir..."
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${GLOBAL_ASSETS_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${LOCAL_ASSETS_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/assets"
    COMMENT "Copying assets directory for map_editor"
    VERBATIM
)

add_dependencies(map_editor copy_map_editor_assets)
