# ==============================================================================
# Monster Math - Standalone Math Library
# ==============================================================================

cmake_minimum_required(VERSION 3.16)

# Header-only interface library
add_library(mmath INTERFACE)

# GLM is bundled with Monster Math
set(MMATH_GLM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm)

target_include_directories(mmath INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${MMATH_GLM_DIR}
)

target_compile_definitions(mmath INTERFACE
    GLM_ENABLE_EXPERIMENTAL
)

# C++17 minimum for std::optional, if constexpr, etc.
target_compile_features(mmath INTERFACE cxx_std_17)
