
#include <engine/core/Log.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace se {
static std::shared_ptr<spdlog::logger> g_logger;

void LogInit(bool enableDebugLogs, bool toFile) {
    // Prevent double initialization
    if (g_logger) return;
    
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

    if (toFile) { 
        sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/engine.log", 1024 * 1024 * 5, 3)); 
    }

    g_logger = std::make_shared<spdlog::logger>("engine", begin(sinks), end(sinks));
    spdlog::register_logger(g_logger);
    
    if (enableDebugLogs) {
        g_logger->set_level(spdlog::level::debug);
    } else {
        g_logger->set_level(spdlog::level::info);
    }
    g_logger->flush_on(spdlog::level::err);
}

std::shared_ptr<spdlog::logger>& Logger() {
    // Safety: if logger not initialized, do it now with defaults
    if (!g_logger) {
        LogInit(false, false);
    }
    return g_logger;
}
}  // namespace se
