cmake_minimum_required(VERSION 3.16)

project(animation_editor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

add_executable(animation_editor ${SOURCES})

target_include_directories(animation_editor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET animation_editor PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

target_link_libraries(animation_editor PRIVATE
    simple_engine
    ImGui
    imnodes
)

# Copy assets from global assets
set(GLOBAL_ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")

add_custom_command(
    TARGET animation_editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying assets to animation_editor binary dir..."
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:animation_editor>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${GLOBAL_ASSETS_DIR}" "$<TARGET_FILE_DIR:animation_editor>/assets"
    COMMENT "Copying assets directory for animation_editor"
    VERBATIM
)
