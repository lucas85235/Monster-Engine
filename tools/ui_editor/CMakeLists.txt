cmake_minimum_required(VERSION 3.16)

project(ui_editor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

add_executable(ui_editor ${SOURCES})

target_include_directories(ui_editor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET ui_editor PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

target_link_libraries(ui_editor PRIVATE
    simple_engine
    ImGui
)

# Copy assets from both global assets and local ui_editor assets
set(GLOBAL_ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")
set(LOCAL_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")

# Use POST_BUILD to copy assets to the directory containing the executable
add_custom_command(TARGET ui_editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying assets to ui_editor output dir..."
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:ui_editor>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${GLOBAL_ASSETS_DIR}" "$<TARGET_FILE_DIR:ui_editor>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${LOCAL_ASSETS_DIR}" "$<TARGET_FILE_DIR:ui_editor>/assets"
    COMMENT "Copying assets directory for ui_editor"
    VERBATIM
)

