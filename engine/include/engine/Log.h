#pragma once
// clang-format off
#include "Engine.h"
#include <memory>
#include <spdlog/spdlog.h>

namespace se {
void LogInit(bool toFile = true);

    // returns the global logger object
    std::shared_ptr<spdlog::logger>& Logger();
} // namespace se

#ifdef DEBUG
#define SE_LOG_INFO(...) ::se::Logger()->info(__VA_ARGS__)
#define SE_LOG_WARN(...) ::se::Logger()->warn(__VA_ARGS__)
#define SE_LOG_ERROR(...) ::se::Logger()->error(__VA_ARGS__)
#define SE_LOG_DEBUG(...) ::se::Logger()->debug(__VA_ARGS__)
#define SE_LOG_CRITICAL(...) ::se::Logger()->critical(__VA_ARGS__)
#else
#define SE_LOG_INFO(...)     do {} while (false)
#define SE_LOG_WARN(...)     do {} while (false)
#define SE_LOG_ERROR(...)    do {} while (false)
#define SE_LOG_DEBUG(...)    do {} while (false)
#define SE_LOG_CRITICAL(...) do {} while (false)
// clang-format on
#endif
