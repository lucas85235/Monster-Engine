cmake_minimum_required(VERSION 3.5)

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
file(GLOB_RECURSE SOURCES
        "${SRC_DIR}/*.cpp"
)


add_executable(third_person_game
        ${SOURCES})

target_include_directories(third_person_game PUBLIC

)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET ImGui PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
set_property(TARGET third_person_game PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

target_link_libraries(third_person_game
        simple_engine
        ImGui
)

set(ASSETS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets")

if (EXISTS "${ASSETS_SOURCE_DIR}")
    file(GLOB_RECURSE SHADER_FILES
            "${ASSETS_SOURCE_DIR}/shaders/*.vert"
            "${ASSETS_SOURCE_DIR}/shaders/*.frag"
    )

    add_custom_target(copy_assets ALL DEPENDS ${SHADER_FILES})

    add_custom_command(
            TARGET copy_assets
            COMMAND ${CMAKE_COMMAND} -E echo "Assets changed, copying to binary dir..."
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/assets"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${ASSETS_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/assets"
            COMMENT "Copying assets directory"
            VERBATIM
    )

    add_dependencies(third_person_game copy_assets)
else ()
    message(WARNING "Assets dir not found: ${ASSETS_SOURCE_DIR}")
endif ()