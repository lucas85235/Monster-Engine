#pragma once

#ifdef _WIN32
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <Windows.h>
#endif

// Diga ao GLFW para não incluir headers GL antigos
// #ifndef GLFW_INCLUDE_NONE
// #define GLFW_INCLUDE_NONE
// #endif
// #include <GLFW/glfw3.h>

#include <glad/glad.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "engine/Log.h"
#include "imgui.h"
