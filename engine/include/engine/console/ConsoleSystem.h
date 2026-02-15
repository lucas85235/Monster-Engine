#pragma once

#include <deque>
#include <filesystem>
#include <functional>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace spdlog {
namespace sinks {
class sink;
}
}  // namespace spdlog

namespace se {

enum class ConsoleVarType {
    Bool = 0,
    Int,
    Float,
    String,
};

struct ConsoleCommand {
    std::string name;
    std::string description;
    std::string usage;
    std::function<void(const std::vector<std::string>& args)> callback;
};

struct ConsoleVar {
    using Value = std::variant<bool, int, float, std::string>;

    std::string name;
    std::string description;
    ConsoleVarType type = ConsoleVarType::String;
    bool archive = false;

    Value value;
    Value defaultValue;

    std::function<void(const ConsoleVar& var)> onChanged;
};

class ConsoleSystem {
   public:
    static ConsoleSystem& Get() {
        static ConsoleSystem instance;
        return instance;
    }

    void Init();
    void Shutdown();

    bool IsInitialized() const {
        return initialized_;
    }

    void RegisterCommand(const std::string& name, const std::string& description,
                         const std::string& usage,
                         std::function<void(const std::vector<std::string>& args)> callback);

    void RegisterBoolCVar(const std::string& name, bool defaultValue,
                          const std::string& description, bool archive,
                          std::function<void(const ConsoleVar& var)> onChanged = {});
    void RegisterIntCVar(const std::string& name, int defaultValue,
                         const std::string& description, bool archive,
                         std::function<void(const ConsoleVar& var)> onChanged = {});
    void RegisterFloatCVar(const std::string& name, float defaultValue,
                           const std::string& description, bool archive,
                           std::function<void(const ConsoleVar& var)> onChanged = {});
    void RegisterStringCVar(const std::string& name, const std::string& defaultValue,
                            const std::string& description, bool archive,
                            std::function<void(const ConsoleVar& var)> onChanged = {});

    bool Execute(const std::string& line);

    void AddOutput(const std::string& line);
    void ClearOutput();

    std::vector<std::string> GetOutputLinesSnapshot() const;
    std::vector<std::string> GetOutputLinesRangeSnapshot(size_t begin, size_t count) const;
    std::vector<std::string> GetCommandHistorySnapshot() const;
    size_t                   GetOutputLineCount() const;
    uint64_t                 GetOutputVersion() const;

    std::vector<std::string> AutoComplete(const std::string& prefix) const;

    void SetVisible(bool visible);
    void ToggleVisible();
    bool IsVisible() const {
        return visible_;
    }

    bool HasCommand(const std::string& name) const;
    bool HasCVar(const std::string& name) const;

    bool SetCVarFromString(const std::string& name, const std::string& valueString);
    std::string GetCVarAsString(const std::string& name) const;

    bool SaveConfig(const std::filesystem::path& path) const;
    bool LoadConfig(const std::filesystem::path& path);

    const std::filesystem::path& GetDefaultInputBindingsPath() const {
        return defaultInputBindingsPath_;
    }

    const std::filesystem::path& GetDefaultConsoleConfigPath() const {
        return defaultConsoleConfigPath_;
    }

   private:
    ConsoleSystem() = default;

    static std::vector<std::string> Tokenize(const std::string& line);
    static std::string              Join(const std::vector<std::string>& values, size_t firstIndex);
    static std::string              ToLower(std::string value);

    ConsoleVar*       FindCVar(const std::string& name);
    const ConsoleVar* FindCVar(const std::string& name) const;

    bool SetCVarImpl(ConsoleVar& var, const std::string& valueString);

    std::string CVarValueToString(const ConsoleVar& var) const;

    void RegisterBuiltIns();
    void RegisterDefaultCVars();
    void EnsureDefaultBindings();
    void SyncConsoleContextState();
    void AttachLogSink();
    void DetachLogSink();

    std::string engineMapName_        = "engine";
    std::string engineConsoleMapName_ = "engine.console";

    bool initialized_ = false;
    bool visible_     = false;

    std::unordered_map<std::string, ConsoleCommand> commands_;
    std::unordered_map<std::string, ConsoleVar>     cvars_;

    mutable std::mutex       outputMutex_;
    std::deque<std::string>  outputLines_;
    std::vector<std::string> commandHistory_;
    uint64_t                 outputVersion_ = 1;
    std::shared_ptr<spdlog::sinks::sink> logSink_;

    // Deduplication: collapse identical consecutive output lines into "(xN)".
    std::string lastOutputLine_;
    size_t      lastOutputRepeatCount_ = 0;

    std::filesystem::path defaultInputBindingsPath_ = "assets/config/input_bindings.json";
    std::filesystem::path defaultConsoleConfigPath_ = "assets/config/console.cfg";
};

}  // namespace se
