#pragma once
// clang-format off
#include "Engine.h"
#include <memory>
#include <spdlog/spdlog.h>

namespace se {
void LogInit(bool enableDebugLogs = false, bool toFile = false);

// Returns the global logger object
std::shared_ptr<spdlog::logger>& Logger();
} // namespace se

// Logging macros - always available
#define SE_LOG_INFO(...) ::se::Logger()->info(__VA_ARGS__)
#define SE_LOG_WARN(...) ::se::Logger()->warn(__VA_ARGS__)
#define SE_LOG_ERROR(...) ::se::Logger()->error(__VA_ARGS__)
#define SE_LOG_CRITICAL(...) ::se::Logger()->critical(__VA_ARGS__)

// Debug logging - only in debug builds for performance
#ifdef SE_DEBUG
#define SE_LOG_DEBUG(...) ::se::Logger()->debug(__VA_ARGS__)
#else
#define SE_LOG_DEBUG(...) ((void)0)
#endif
// clang-format on
