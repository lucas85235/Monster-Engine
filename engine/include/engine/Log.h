#pragma once
#include <memory>
#include <spdlog/spdlog.h>

namespace se {
void LogInit(bool toFile = true);
std::shared_ptr<spdlog::logger>& Logger(); // retorna o logger global
} // namespace se

// macros convenientes
#define SE_LOG_INFO(...) ::se::Logger()->info(__VA_ARGS__)
#define SE_LOG_WARN(...) ::se::Logger()->warn(__VA_ARGS__)
#define SE_LOG_ERROR(...) ::se::Logger()->error(__VA_ARGS__)
#define SE_LOG_DEBUG(...) ::se::Logger()->debug(__VA_ARGS__)
