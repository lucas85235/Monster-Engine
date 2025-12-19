cmake_minimum_required(VERSION 3.16)
project(SimpleEngineProject LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(UNIX)
# Removed -Wno-changes-meaning as it's not compatible with Clang
# add_compile_options(-Wno-changes-meaning)
endif()

add_subdirectory(engine)
add_subdirectory(apps/sandbox)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  PROPERTY VS_STARTUP_PROJECT sandbox)
