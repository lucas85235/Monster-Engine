#pragma once
// clang-format off
#include <memory>
#include <spdlog/spdlog.h>

namespace se {
void LogInit(bool toFile = true);

    // returns the global logger object
    std::shared_ptr<spdlog::logger>& Logger();
} // namespace se

#if defined(DEBUG) || defined(_DEBUG)
// Debug build - all log levels active
#define SE_LOG_INFO(...) ::se::Logger()->info(__VA_ARGS__)
#define SE_LOG_WARN(...) ::se::Logger()->warn(__VA_ARGS__)
#define SE_LOG_ERROR(...) ::se::Logger()->error(__VA_ARGS__)
#define SE_LOG_DEBUG(...) ::se::Logger()->debug(__VA_ARGS__)
#define SE_LOG_CRITICAL(...) ::se::Logger()->critical(__VA_ARGS__)
#else
// Release build - strip INFO and DEBUG for performance
#define SE_LOG_INFO(...)     do { } while(0)
#define SE_LOG_DEBUG(...)    do { } while(0)
// Keep WARN, ERROR, and CRITICAL for production issues
#define SE_LOG_WARN(...)     ::se::Logger()->warn(__VA_ARGS__)
#define SE_LOG_ERROR(...)    ::se::Logger()->error(__VA_ARGS__)
#define SE_LOG_CRITICAL(...) ::se::Logger()->critical(__VA_ARGS__)
// clang-format on
#endif
