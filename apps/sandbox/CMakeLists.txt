add_executable(sandbox
        src/AppLayer.cpp
        src/AppLayer.h
        src/input_sample/InputSampleLayer.h
        src/input_sample/InputSampleLayer.cpp
        src/event_sample/EventSampleLayer.cpp
        src/event_sample/EventSampleLayer.h
        src/event_sample/events/Events.h
        src/SampleUtilities.h
        src/main.cpp)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET ImGui PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
set_property(TARGET sandbox PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

target_link_libraries(sandbox PUBLIC simple_engine)

target_compile_definitions(sandbox PUBLIC PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

set(ASSETS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets")

if(EXISTS "${ASSETS_SOURCE_DIR}")
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

    add_dependencies(sandbox copy_assets)
else()
    message(WARNING "Assets dir not found: ${ASSETS_SOURCE_DIR}")
endif()
