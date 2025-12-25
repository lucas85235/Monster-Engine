cmake_minimum_required(VERSION 3.16)
project(SimpleEngineProject LANGUAGES CXX C)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(engine)
#add_subdirectory(apps/sandbox)
add_subdirectory(apps/third_person_game)
add_subdirectory(tools/map_editor)
add_subdirectory(tools/ui_editor)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        PROPERTY VS_STARTUP_PROJECT sandbox)
