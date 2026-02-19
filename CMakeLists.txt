cmake_minimum_required(VERSION 3.16)
project(SimpleEngineProject LANGUAGES CXX C)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Read version from ENGINE_VERSION file
file(READ "${CMAKE_SOURCE_DIR}/ENGINE_VERSION" VERSION_CONTENT)
string(STRIP "${VERSION_CONTENT}" VERSION_STRING)
string(REPLACE "." ";" VERSION_LIST ${VERSION_STRING})
list(GET VERSION_LIST 0 VERSION_MAJOR)
list(GET VERSION_LIST 1 VERSION_MINOR)
list(GET VERSION_LIST 2 VERSION_PATCH)

# Get git hash
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT GIT_HASH)
    set(GIT_HASH "unknown")
endif()

# Set build type string
if(NOT CMAKE_BUILD_TYPE)
    set(BUILD_TYPE "Debug")
else()
    set(BUILD_TYPE ${CMAKE_BUILD_TYPE})
endif()

# Generate Version.h
configure_file(
    "${CMAKE_SOURCE_DIR}/engine/include/Version.h.in"
    "${CMAKE_SOURCE_DIR}/engine/include/Version.h"
    @ONLY
)

message(STATUS "SimpleEngine v${VERSION_STRING} (${GIT_HASH}) - ${BUILD_TYPE}")
add_subdirectory(libs/mmath)
add_subdirectory(engine)
option(SE_BUILD_APP_SANDBOX "Build sandbox app" ON)
option(SE_BUILD_APP_ANIMATION_TEST "Build animation_test app" OFF)
option(SE_BUILD_APP_DEMOTEST "Build demo_test app" OFF)

if(SE_BUILD_APP_SANDBOX)
    add_subdirectory(apps/sandbox)
endif()

if (SE_BUILD_APP_DEMOTEST)
    add_subdirectory(apps/demo_test)
endif ()

# Temporarily disabled — need Filament rendering pipeline reimplemented
#add_subdirectory(apps/third_person_game)
if(SE_BUILD_APP_ANIMATION_TEST)
    add_subdirectory(apps/animation_test)
endif()
#add_subdirectory(tools/map_editor)
#add_subdirectory(tools/ui_editor)
#add_subdirectory(tools/animation_editor)

if(SE_BUILD_APP_SANDBOX)
    set(SE_VS_STARTUP_PROJECT sandbox)
elseif(SE_BUILD_APP_ANIMATION_TEST)
    set(SE_VS_STARTUP_PROJECT animation_test)
elseif (SE_BUILD_APP_DEMOTEST)
    set(SE_VS_STARTUP_PROJECT demo_test)
endif()

if(DEFINED SE_VS_STARTUP_PROJECT)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            PROPERTY VS_STARTUP_PROJECT ${SE_VS_STARTUP_PROJECT})
endif()
