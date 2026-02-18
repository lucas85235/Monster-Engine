cmake_minimum_required(VERSION 3.5)

# Flutter integration test application
add_executable(flutter_test
        src/main.cpp
        src/FlutterTestLayer.cpp)

set_property(TARGET flutter_test PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

target_link_libraries(flutter_test PUBLIC simple_engine)

target_compile_definitions(flutter_test PUBLIC PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

set(ASSETS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets")

if (EXISTS "${ASSETS_SOURCE_DIR}")
    add_custom_target(copy_flutter_test_assets ALL)

    add_custom_command(
            TARGET copy_flutter_test_assets
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/assets"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${ASSETS_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/assets"
            COMMENT "Copying assets for flutter_test"
            VERBATIM
    )

    add_dependencies(flutter_test copy_flutter_test_assets)
endif ()

# Copy Flutter app build artifacts next to the executable
if (SE_ENABLE_FLUTTER)
    set(FLUTTER_APP_BUILD_DIR "${CMAKE_SOURCE_DIR}/apps/flutter_test/flutter_app/build")
    if (EXISTS "${FLUTTER_APP_BUILD_DIR}")
        add_custom_command(
                TARGET flutter_test POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:flutter_test>/flutter_app"
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${FLUTTER_APP_BUILD_DIR}" "$<TARGET_FILE_DIR:flutter_test>/flutter_app"
                COMMENT "Copying Flutter app build artifacts"
                VERBATIM
        )
    endif ()
endif ()
