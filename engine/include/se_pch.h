#pragma once

// ============================================================================
// Platform-specific
// ============================================================================
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

// ============================================================================
// Standard Library
// ============================================================================
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

// ============================================================================
// GLFW (windowing only — no OpenGL, rendering via Filament)
// ============================================================================
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

// ============================================================================
// Monster Math Library (includes GLM internally)
// ============================================================================
#include <mmath/Luma.h>

// ============================================================================
// EnTT (ECS)
// ============================================================================
#include <entt.hpp>

// ============================================================================
// ImGui
// ============================================================================
#include <imgui.h>

// ============================================================================
// spdlog (Logging)
// ============================================================================
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

// ============================================================================
// Bullet Physics
// ============================================================================
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>

// ============================================================================
// Assimp (Model Loading)
// ============================================================================
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// ============================================================================
// Engine Core
// ============================================================================
#include "engine/Log.h"
#include "Engine.h"
