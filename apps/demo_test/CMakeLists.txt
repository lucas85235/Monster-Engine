cmake_minimum_required(VERSION 3.5)

add_executable(animation_test
        src/main.cpp
        src/layers/AnimationTestLayer.h
        src/layers/AnimationTestLayer.cpp
        src/components/AdvancedCharacterAnimator.h
        src/components/AdvancedCharacterAnimator.cpp
)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET animation_test PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

target_link_libraries(animation_test PUBLIC simple_engine)

target_compile_definitions(animation_test PUBLIC PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

set(ASSETS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets")

if (EXISTS "${ASSETS_SOURCE_DIR}")
    file(GLOB_RECURSE SHADER_FILES
            "${ASSETS_SOURCE_DIR}/shaders/*.vert"
            "${ASSETS_SOURCE_DIR}/shaders/*.frag"
    )

    add_custom_target(copy_assets_animation_test ALL DEPENDS ${SHADER_FILES})

    add_custom_command(
            TARGET copy_assets_animation_test
            COMMAND ${CMAKE_COMMAND} -E echo "Assets changed, copying to binary dir..."
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/assets"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${ASSETS_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/assets"
            COMMENT "Copying assets directory for animation_test"
            VERBATIM
    )

    add_dependencies(animation_test copy_assets_animation_test)
else ()
    message(WARNING "Assets dir not found: ${ASSETS_SOURCE_DIR}")
endif ()
