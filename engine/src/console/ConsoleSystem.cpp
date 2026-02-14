#include "engine/console/ConsoleSystem.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <spdlog/details/log_msg.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/base_sink.h>

#include "engine/Application.h"
#include "engine/Log.h"
#include "engine/core/PerformanceProfiler.h"
#include "engine/core/ServiceLocator.h"
#include "engine/input/InputManager.h"
#include "engine/ui/native/NativeUiRenderer.h"
#include "engine/ui/native/retained/RetainedUi.h"

namespace se {

namespace {

constexpr size_t kMaxConsoleOutputLines  = 2048;
constexpr size_t kMaxConsoleHistoryLines = 256;

class ConsoleSpdlogSink final : public spdlog::sinks::base_sink<std::mutex> {
   public:
    explicit ConsoleSpdlogSink(ConsoleSystem* owner) : owner_(owner) {
        // Keep logs concise and plain-text for the native UI text renderer.
        this->set_pattern("[%H:%M:%S] [%l] %v");
    }

   protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        if (!owner_) return;
        spdlog::memory_buf_t buffer;
        this->formatter_->format(msg, buffer);
        std::string line(buffer.data(), buffer.size());
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
            line.pop_back();
        }
        if (!line.empty()) {
            owner_->AddOutput(line);
        }
    }

    void flush_() override {}

   private:
    ConsoleSystem* owner_ = nullptr;
};

bool ParseBool(const std::string& value, bool* outBool) {
    if (!outBool) return false;

    std::string lower;
    lower.resize(value.size());
    std::transform(value.begin(), value.end(), lower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (lower == "1" || lower == "true" || lower == "on" || lower == "yes") {
        *outBool = true;
        return true;
    }

    if (lower == "0" || lower == "false" || lower == "off" || lower == "no") {
        *outBool = false;
        return true;
    }

    return false;
}

bool ParseInt(const std::string& value, int* outInt) {
    if (!outInt) return false;
    const char* begin = value.data();
    const char* end   = value.data() + value.size();
    auto [ptr, ec]    = std::from_chars(begin, end, *outInt);
    return ec == std::errc{} && ptr == end;
}

bool ParseFloat(const std::string& value, float* outFloat) {
    if (!outFloat) return false;
    std::istringstream stream(value);
    stream >> std::noskipws >> *outFloat;
    return stream && stream.eof();
}

std::string QuoteIfNeeded(const std::string& value) {
    if (value.empty()) return "\"\"";
    if (value.find_first_of(" \t\"") == std::string::npos) return value;

    std::string escaped;
    escaped.reserve(value.size() + 4);
    escaped.push_back('"');
    for (char c : value) {
        if (c == '"' || c == '\\') escaped.push_back('\\');
        escaped.push_back(c);
    }
    escaped.push_back('"');
    return escaped;
}

std::string Trim(const std::string& value) {
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        start++;
    }

    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        end--;
    }

    return value.substr(start, end - start);
}

std::string FormatFloat2(float value) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << value;
    return stream.str();
}

float Percent(float part, float whole) {
    if (whole <= 0.0001f) return 0.0f;
    return (part / whole) * 100.0f;
}

std::vector<std::string> SplitCommands(const std::string& line) {
    std::vector<std::string> commands;
    std::string              current;
    bool                     inQuotes = false;
    bool                     escaped  = false;

    for (char c : line) {
        if (escaped) {
            current.push_back(c);
            escaped = false;
            continue;
        }

        if (c == '\\') {
            current.push_back(c);
            escaped = true;
            continue;
        }

        if (c == '"') {
            current.push_back(c);
            inQuotes = !inQuotes;
            continue;
        }

        if (!inQuotes && c == ';') {
            const std::string trimmed = Trim(current);
            if (!trimmed.empty()) {
                commands.push_back(trimmed);
            }
            current.clear();
            continue;
        }

        current.push_back(c);
    }

    const std::string trimmed = Trim(current);
    if (!trimmed.empty()) {
        commands.push_back(trimmed);
    }

    return commands;
}

}  // namespace

void ConsoleSystem::Init() {
    if (initialized_) return;

    commands_.clear();
    cvars_.clear();
    {
        std::scoped_lock lock(outputMutex_);
        outputLines_.clear();
        commandHistory_.clear();
        outputVersion_ = 1;
        lastOutputLine_.clear();
        lastOutputRepeatCount_ = 0;
    }

    RegisterBuiltIns();
    RegisterDefaultCVars();

    EnsureDefaultBindings();

    LoadConfig(defaultConsoleConfigPath_);
    LoadConfig("assets/config/autoexec.cfg");
    InputManager::Get().LoadBindings(defaultInputBindingsPath_);
    EnsureDefaultBindings();
    visible_ = false;
    SyncConsoleContextState();

    AttachLogSink();
    AddOutput("Developer console initialized.");
    initialized_ = true;
}

void ConsoleSystem::Shutdown() {
    if (!initialized_) return;

    SetVisible(false);
    DetachLogSink();

    SaveConfig(defaultConsoleConfigPath_);
    InputManager::Get().SaveBindings(defaultInputBindingsPath_);

    commands_.clear();
    cvars_.clear();
    {
        std::scoped_lock lock(outputMutex_);
        outputLines_.clear();
        commandHistory_.clear();
        outputVersion_ = 1;
        lastOutputLine_.clear();
        lastOutputRepeatCount_ = 0;
    }

    initialized_ = false;
}

void ConsoleSystem::SetVisible(bool visible) {
    if (visible_ == visible) return;
    visible_ = visible;
    SyncConsoleContextState();
}

void ConsoleSystem::ToggleVisible() {
    visible_ = !visible_;
    SyncConsoleContextState();
}

void ConsoleSystem::RegisterCommand(
    const std::string& name, const std::string& description, const std::string& usage,
    std::function<void(const std::vector<std::string>& args)> callback) {
    if (name.empty() || !callback) return;

    ConsoleCommand command;
    command.name        = name;
    command.description = description;
    command.usage       = usage;
    command.callback    = std::move(callback);

    commands_[ToLower(name)] = std::move(command);
}

void ConsoleSystem::RegisterBoolCVar(const std::string& name, bool defaultValue,
                                     const std::string& description, bool archive,
                                     std::function<void(const ConsoleVar& var)> onChanged) {
    ConsoleVar var;
    var.name         = name;
    var.description  = description;
    var.type         = ConsoleVarType::Bool;
    var.archive      = archive;
    var.value        = defaultValue;
    var.defaultValue = defaultValue;
    var.onChanged    = std::move(onChanged);

    auto& stored = cvars_[ToLower(name)] = std::move(var);
    if (stored.onChanged) stored.onChanged(stored);
}

void ConsoleSystem::RegisterIntCVar(const std::string& name, int defaultValue,
                                    const std::string& description, bool archive,
                                    std::function<void(const ConsoleVar& var)> onChanged) {
    ConsoleVar var;
    var.name         = name;
    var.description  = description;
    var.type         = ConsoleVarType::Int;
    var.archive      = archive;
    var.value        = defaultValue;
    var.defaultValue = defaultValue;
    var.onChanged    = std::move(onChanged);

    auto& stored = cvars_[ToLower(name)] = std::move(var);
    if (stored.onChanged) stored.onChanged(stored);
}

void ConsoleSystem::RegisterFloatCVar(const std::string& name, float defaultValue,
                                      const std::string& description, bool archive,
                                      std::function<void(const ConsoleVar& var)> onChanged) {
    ConsoleVar var;
    var.name         = name;
    var.description  = description;
    var.type         = ConsoleVarType::Float;
    var.archive      = archive;
    var.value        = defaultValue;
    var.defaultValue = defaultValue;
    var.onChanged    = std::move(onChanged);

    auto& stored = cvars_[ToLower(name)] = std::move(var);
    if (stored.onChanged) stored.onChanged(stored);
}

void ConsoleSystem::RegisterStringCVar(const std::string& name,
                                       const std::string& defaultValue,
                                       const std::string& description, bool archive,
                                       std::function<void(const ConsoleVar& var)> onChanged) {
    ConsoleVar var;
    var.name         = name;
    var.description  = description;
    var.type         = ConsoleVarType::String;
    var.archive      = archive;
    var.value        = defaultValue;
    var.defaultValue = defaultValue;
    var.onChanged    = std::move(onChanged);

    auto& stored = cvars_[ToLower(name)] = std::move(var);
    if (stored.onChanged) stored.onChanged(stored);
}

bool ConsoleSystem::Execute(const std::string& line) {
    const auto chainedCommands = SplitCommands(line);
    if (chainedCommands.empty()) return false;
    if (chainedCommands.size() > 1) {
        bool allOk = true;
        for (const auto& commandLine : chainedCommands) {
            allOk = Execute(commandLine) && allOk;
        }
        return allOk;
    }

    const std::string& singleLine = chainedCommands.front();
    if (singleLine.empty()) return false;

    const auto tokens = Tokenize(singleLine);
    if (tokens.empty()) return false;

    {
        std::scoped_lock lock(outputMutex_);
        commandHistory_.push_back(singleLine);
        if (commandHistory_.size() > kMaxConsoleHistoryLines) {
            commandHistory_.erase(
                commandHistory_.begin(),
                commandHistory_.begin() + (commandHistory_.size() - kMaxConsoleHistoryLines));
        }
    }

    const std::string commandKey = ToLower(tokens.front());
    std::vector<std::string> args;
    args.reserve(tokens.size() > 1 ? tokens.size() - 1 : 0);
    for (size_t i = 1; i < tokens.size(); i++) {
        args.push_back(tokens[i]);
    }

    auto commandIt = commands_.find(commandKey);
    if (commandIt != commands_.end()) {
        commandIt->second.callback(args);
        return true;
    }

    if (ConsoleVar* var = FindCVar(commandKey)) {
        if (args.empty()) {
            AddOutput(var->name + " = " + CVarValueToString(*var));
            return true;
        }

        const std::string value = Join(args, 0);
        if (!SetCVarImpl(*var, value)) {
            AddOutput("Failed to set cvar '" + var->name + "' from value '" + value + "'.");
            return false;
        }

        AddOutput(var->name + " = " + CVarValueToString(*var));
        return true;
    }

    AddOutput("Unknown command or cvar: " + tokens.front());
    return false;
}

void ConsoleSystem::AddOutput(const std::string& line) {
    std::scoped_lock lock(outputMutex_);

    // Deduplication: collapse identical consecutive lines into "(xN)" suffix.
    if (line == lastOutputLine_) {
        ++lastOutputRepeatCount_;
        // Update the last line in-place with the repeat count.
        if (!outputLines_.empty()) {
            outputLines_.back() = line + " (x" + std::to_string(lastOutputRepeatCount_) + ")";
        }
        ++outputVersion_;
        return;
    }

    // New distinct line — flush previous dedup state.
    lastOutputLine_        = line;
    lastOutputRepeatCount_ = 1;

    outputLines_.push_back(line);
    // Deque pop_front is O(1) — no more O(n) erase-from-begin.
    while (outputLines_.size() > kMaxConsoleOutputLines) {
        outputLines_.pop_front();
    }
    ++outputVersion_;
}

void ConsoleSystem::ClearOutput() {
    std::scoped_lock lock(outputMutex_);
    outputLines_.clear();
    lastOutputLine_.clear();
    lastOutputRepeatCount_ = 0;
    ++outputVersion_;
}

std::vector<std::string> ConsoleSystem::GetOutputLinesSnapshot() const {
    std::scoped_lock lock(outputMutex_);
    return {outputLines_.begin(), outputLines_.end()};
}

std::vector<std::string> ConsoleSystem::GetOutputLinesRangeSnapshot(size_t begin,
                                                                     size_t count) const {
    std::scoped_lock lock(outputMutex_);

    if (begin >= outputLines_.size() || count == 0) {
        return {};
    }

    const size_t end = std::min(outputLines_.size(), begin + count);
    std::vector<std::string> lines;
    lines.reserve(end - begin);
    for (size_t i = begin; i < end; ++i) {
        lines.push_back(outputLines_[i]);
    }
    return lines;
}

std::vector<std::string> ConsoleSystem::GetCommandHistorySnapshot() const {
    std::scoped_lock lock(outputMutex_);
    return commandHistory_;
}

size_t ConsoleSystem::GetOutputLineCount() const {
    std::scoped_lock lock(outputMutex_);
    return outputLines_.size();
}

uint64_t ConsoleSystem::GetOutputVersion() const {
    std::scoped_lock lock(outputMutex_);
    return outputVersion_;
}

std::vector<std::string> ConsoleSystem::AutoComplete(const std::string& prefix) const {
    const std::string lowerPrefix = ToLower(prefix);

    std::vector<std::string> matches;

    for (const auto& [name, command] : commands_) {
        if (name.rfind(lowerPrefix, 0) == 0) {
            matches.push_back(command.name);
        }
    }

    for (const auto& [name, cvar] : cvars_) {
        if (name.rfind(lowerPrefix, 0) == 0) {
            matches.push_back(cvar.name);
        }
    }

    std::sort(matches.begin(), matches.end());
    matches.erase(std::unique(matches.begin(), matches.end()), matches.end());
    return matches;
}

bool ConsoleSystem::HasCommand(const std::string& name) const {
    return commands_.contains(ToLower(name));
}

bool ConsoleSystem::HasCVar(const std::string& name) const {
    return cvars_.contains(ToLower(name));
}

bool ConsoleSystem::SetCVarFromString(const std::string& name, const std::string& valueString) {
    ConsoleVar* var = FindCVar(name);
    if (!var) return false;
    return SetCVarImpl(*var, valueString);
}

std::string ConsoleSystem::GetCVarAsString(const std::string& name) const {
    const ConsoleVar* var = FindCVar(name);
    if (!var) return "";
    return CVarValueToString(*var);
}

bool ConsoleSystem::SaveConfig(const std::filesystem::path& path) const {
    try {
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }

        std::ofstream output(path);
        if (!output.is_open()) {
            SE_LOG_ERROR("Failed to open console config file for write: {}", path.string());
            return false;
        }

        output << "// Monster Engine console config\n";

        std::vector<const ConsoleVar*> vars;
        vars.reserve(cvars_.size());
        for (const auto& [name, var] : cvars_) {
            if (var.archive) vars.push_back(&var);
        }
        std::sort(vars.begin(), vars.end(), [](const ConsoleVar* a, const ConsoleVar* b) {
            return a->name < b->name;
        });

        for (const ConsoleVar* var : vars) {
            std::string value = CVarValueToString(*var);
            if (var->type == ConsoleVarType::String) {
                value = QuoteIfNeeded(value);
            }
            output << var->name << " " << value << "\n";
        }

        return true;
    } catch (const std::exception& e) {
        SE_LOG_ERROR("Failed to save console config: {}", e.what());
        return false;
    }
}

bool ConsoleSystem::LoadConfig(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(input, line)) {
        const std::string trimmedLine = Trim(line);
        if (trimmedLine.empty()) continue;

        const auto lowered = ToLower(trimmedLine);
        if (lowered.rfind("//", 0) == 0) continue;
        if (lowered.rfind("#", 0) == 0) continue;

        const auto tokens = Tokenize(trimmedLine);
        if (tokens.empty()) continue;

        const std::string key = ToLower(tokens[0]);
        ConsoleVar* var        = FindCVar(key);
        if (!var) {
            // For flexibility, allow command execution from config as well.
            Execute(line);
            continue;
        }

        const std::string value = Join(tokens, 1);
        SetCVarImpl(*var, value);
    }

    return true;
}

std::vector<std::string> ConsoleSystem::Tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::string current;

    bool inQuotes = false;
    bool escaped  = false;

    for (char c : line) {
        if (escaped) {
            current.push_back(c);
            escaped = false;
            continue;
        }

        if (c == '\\') {
            escaped = true;
            continue;
        }

        if (c == '"') {
            inQuotes = !inQuotes;
            continue;
        }

        if (!inQuotes && std::isspace(static_cast<unsigned char>(c))) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }

        current.push_back(c);
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }

    return tokens;
}

std::string ConsoleSystem::Join(const std::vector<std::string>& values, size_t firstIndex) {
    if (firstIndex >= values.size()) return "";

    std::string result;
    for (size_t i = firstIndex; i < values.size(); i++) {
        if (i > firstIndex) result.push_back(' ');
        result += values[i];
    }
    return result;
}

std::string ConsoleSystem::ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

ConsoleVar* ConsoleSystem::FindCVar(const std::string& name) {
    auto it = cvars_.find(ToLower(name));
    if (it == cvars_.end()) return nullptr;
    return &it->second;
}

const ConsoleVar* ConsoleSystem::FindCVar(const std::string& name) const {
    auto it = cvars_.find(ToLower(name));
    if (it == cvars_.end()) return nullptr;
    return &it->second;
}

bool ConsoleSystem::SetCVarImpl(ConsoleVar& var, const std::string& valueString) {
    switch (var.type) {
        case ConsoleVarType::Bool: {
            bool value = false;
            if (!ParseBool(valueString, &value)) return false;
            var.value = value;
            break;
        }
        case ConsoleVarType::Int: {
            int value = 0;
            if (!ParseInt(valueString, &value)) return false;
            var.value = value;
            break;
        }
        case ConsoleVarType::Float: {
            float value = 0.0f;
            if (!ParseFloat(valueString, &value)) return false;
            var.value = value;
            break;
        }
        case ConsoleVarType::String:
            var.value = valueString;
            break;
    }

    if (var.onChanged) var.onChanged(var);
    return true;
}

std::string ConsoleSystem::CVarValueToString(const ConsoleVar& var) const {
    switch (var.type) {
        case ConsoleVarType::Bool:
            return std::get<bool>(var.value) ? "1" : "0";
        case ConsoleVarType::Int:
            return std::to_string(std::get<int>(var.value));
        case ConsoleVarType::Float: {
            std::ostringstream stream;
            stream << std::get<float>(var.value);
            return stream.str();
        }
        case ConsoleVarType::String:
            return std::get<std::string>(var.value);
    }

    return "";
}

void ConsoleSystem::RegisterBuiltIns() {
    RegisterCommand(
        "help", "List commands and cvars", "help [filter]",
        [this](const std::vector<std::string>& args) {
            const std::string filter = args.empty() ? "" : ToLower(args[0]);

            AddOutput("--- Commands ---");
            std::vector<const ConsoleCommand*> commandList;
            commandList.reserve(commands_.size());
            for (const auto& [key, cmd] : commands_) {
                if (!filter.empty() && key.find(filter) == std::string::npos) continue;
                commandList.push_back(&cmd);
            }
            std::sort(commandList.begin(), commandList.end(), [](const ConsoleCommand* a, const ConsoleCommand* b) {
                return a->name < b->name;
            });
            for (const ConsoleCommand* cmd : commandList) {
                AddOutput(cmd->name + " - " + cmd->description);
                if (!cmd->usage.empty()) AddOutput("  usage: " + cmd->usage);
            }

            AddOutput("--- CVars ---");
            std::vector<const ConsoleVar*> cvarList;
            cvarList.reserve(cvars_.size());
            for (const auto& [key, var] : cvars_) {
                if (!filter.empty() && key.find(filter) == std::string::npos) continue;
                cvarList.push_back(&var);
            }
            std::sort(cvarList.begin(), cvarList.end(), [](const ConsoleVar* a, const ConsoleVar* b) {
                return a->name < b->name;
            });
            for (const ConsoleVar* var : cvarList) {
                AddOutput(var->name + " = " + CVarValueToString(*var) + " - " + var->description);
            }
        });

    RegisterCommand("find", "Alias for help with filter", "find <pattern>",
                    [this](const std::vector<std::string>& args) {
                        if (args.empty()) {
                            AddOutput("usage: find <pattern>");
                            return;
                        }
                        Execute("help " + args[0]);
                    });

    RegisterCommand("clear", "Clear console output", "clear",
                    [this](const std::vector<std::string>& /*args*/) { ClearOutput(); });

    RegisterCommand("ui.stats", "Print native UI renderer frame stats", "ui.stats",
                    [this](const std::vector<std::string>& /*args*/) {
                        const auto& stats = ui::NativeUiRenderer::Get().GetLastFrameStats();
                        AddOutput("ui.stats:");
                        AddOutput("  draw_calls=" + std::to_string(stats.drawCallsIssued));
                        AddOutput("  quads_submitted=" + std::to_string(stats.quadsSubmitted));
                        AddOutput("  vertices_uploaded=" + std::to_string(stats.verticesUploaded));
                        AddOutput("  indices_uploaded=" + std::to_string(stats.indicesUploaded));
                        AddOutput(std::string("  geometry_uploaded=") +
                                  (stats.geometryUploaded ? "1" : "0"));
                        AddOutput(std::string("  geometry_reused=") +
                                  (stats.geometryReused ? "1" : "0"));
                    });

    RegisterCommand("ui.invalidate", "Force native UI cache rebuild", "ui.invalidate",
                    [this](const std::vector<std::string>& /*args*/) {
                        ui::NativeUiRenderer::Get().InvalidateRetainedGeometry();
                        AddOutput("ui.invalidate: ok");
                    });

    RegisterCommand("ui.retained.stats", "Print retained UI document stats", "ui.retained.stats",
                    [this](const std::vector<std::string>& /*args*/) {
                        const auto& stats = ui::retained::RetainedUiContext::Get().GetStats();
                        AddOutput("ui.retained.stats:");
                        AddOutput("  nodes=" + std::to_string(stats.nodeCount));
                        AddOutput("  draw_commands=" + std::to_string(stats.drawCommandCount));
                        AddOutput("  layout_passes=" + std::to_string(stats.layoutPasses));
                        AddOutput("  paint_passes=" + std::to_string(stats.paintPasses));
                        AddOutput("  dirty_frames=" + std::to_string(stats.dirtyFrames));
                        AddOutput("  uploaded_frames=" + std::to_string(stats.uploadedFrames));
                        AddOutput("  reused_frames=" + std::to_string(stats.reusedFrames));
                    });

    RegisterCommand("ui.retained.reset", "Reset retained UI document tree", "ui.retained.reset",
                    [this](const std::vector<std::string>& /*args*/) {
                        auto& retained = ui::retained::RetainedUiContext::Get();
                        if (!retained.IsInitialized()) {
                            retained.Init();
                        }
                        retained.Reset();
                        AddOutput("ui.retained.reset: ok");
                    });

    RegisterCommand("perf.stats", "Print CPU frame timing breakdown", "perf.stats",
                    [this](const std::vector<std::string>& /*args*/) {
                        const auto& profiler = PerformanceProfiler::Get();
                        const auto& current  = profiler.GetMetrics();
                        const auto& average  = profiler.GetAveragedMetrics();
                        const auto& uiStats  = ui::NativeUiRenderer::Get().GetLastFrameStats();

                        AddOutput("perf.stats:");
                        AddOutput("  fps=" + FormatFloat2(profiler.GetFPS()) +
                                  " frame_ms=" + FormatFloat2(current.frameTimeMs) +
                                  " avg_frame_ms=" + FormatFloat2(average.frameTimeMs));

                        AddOutput("  update: " +
                                  FormatFloat2(current.updateTimeMs) + "ms (" +
                                  FormatFloat2(Percent(current.updateTimeMs, current.frameTimeMs)) + "%)");
                        AddOutput("    input=" + FormatFloat2(current.inputUpdateTimeMs) +
                                  " event_pump=" + FormatFloat2(current.eventPumpTimeMs) +
                                  " settings=" + FormatFloat2(current.settingsApplyTimeMs) +
                                  " resize=" + FormatFloat2(current.resizeHandlingTimeMs));

                        AddOutput("  render: " +
                                  FormatFloat2(current.renderTimeMs) + "ms (" +
                                  FormatFloat2(Percent(current.renderTimeMs, current.frameTimeMs)) + "%)");
                        AddOutput("    setup=" + FormatFloat2(current.renderSetupTimeMs) +
                                  " layer_update=" + FormatFloat2(current.layerUpdateTimeMs) +
                                  " anim=" + FormatFloat2(current.animationTimeMs) +
                                  " layer_render=" + FormatFloat2(current.layerRenderTimeMs) +
                                  " ui_end=" + FormatFloat2(current.uiEndFrameTimeMs) +
                                  " present=" + FormatFloat2(current.presentTimeMs));

                        AddOutput("  frame_limiter=" + FormatFloat2(current.frameLimiterTimeMs) + "ms");
                        AddOutput("  physics_ms=" + FormatFloat2(current.physicsTimeMs) +
                                  " active_bodies=" + std::to_string(current.activePhysicsBodies));

                        AddOutput("  ui: draw_calls=" + std::to_string(uiStats.drawCallsIssued) +
                                  " quads=" + std::to_string(uiStats.quadsSubmitted) +
                                  " uploaded=" + (uiStats.geometryUploaded ? "1" : "0") +
                                  " reused=" + (uiStats.geometryReused ? "1" : "0"));

                        std::array<std::pair<const char*, float>, 6> topStages = {{
                            {"present", average.presentTimeMs},
                            {"layer_render", average.layerRenderTimeMs},
                            {"layer_update", average.layerUpdateTimeMs},
                            {"ui_end", average.uiEndFrameTimeMs},
                            {"event_pump", average.eventPumpTimeMs},
                            {"frame_limiter", average.frameLimiterTimeMs},
                        }};
                        std::sort(topStages.begin(), topStages.end(),
                                  [](const auto& a, const auto& b) { return a.second > b.second; });
                        AddOutput("  top_avg_stages:");
                        for (const auto& [name, ms] : topStages) {
                            AddOutput("    " + std::string(name) + "=" + FormatFloat2(ms) + "ms");
                        }
                    });

    RegisterCommand("perf.reset", "Reset performance counters and moving averages", "perf.reset",
                    [this](const std::vector<std::string>& /*args*/) {
                        PerformanceProfiler::Get().ResetStats();
                        AddOutput("perf.reset: ok");
                    });

    RegisterCommand("console.capture_logs",
                    "Enable/disable routing engine logs to console output",
                    "console.capture_logs [0|1]",
                    [this](const std::vector<std::string>& args) {
                        if (args.empty()) {
                            AddOutput(std::string("console.capture_logs = ") +
                                      (logSink_ ? "1" : "0"));
                            return;
                        }

                        bool enabled = false;
                        if (!ParseBool(args[0], &enabled)) {
                            AddOutput("usage: console.capture_logs [0|1]");
                            return;
                        }

                        if (enabled) {
                            AttachLogSink();
                        } else {
                            DetachLogSink();
                        }

                        AddOutput(std::string("console.capture_logs = ") +
                                  (logSink_ ? "1" : "0"));
                    });

    RegisterCommand("ui.retained.demo", "Build a retained UI component showcase",
                    "ui.retained.demo",
                    [this](const std::vector<std::string>& /*args*/) {
                        using namespace ui::retained;

                        auto& retained = RetainedUiContext::Get();
                        if (!retained.IsInitialized()) {
                            retained.Init();
                        }
                        retained.Reset();

                        constexpr UiId kWindow = 1000;
                        constexpr UiId kRow1 = 1010;
                        constexpr UiId kRow2 = 1020;
                        constexpr UiId kRow3 = 1030;
                        constexpr UiId kDataList = 1040;
                        constexpr UiId kFooter = 1050;

                        auto& window = retained.EnsureWindow(kWindow, kInvalidId, "Retained UI Lab");
                        LayoutStyle windowLayout = window.layout;
                        windowLayout.mode = LayoutMode::VStack;
                        windowLayout.x = 48.0f;
                        windowLayout.y = 48.0f;
                        windowLayout.width = 560.0f;
                        windowLayout.height = 640.0f;
                        windowLayout.fillX = false;
                        windowLayout.fillY = false;
                        windowLayout.spacing = 8.0f;
                        windowLayout.padding = 10.0f;
                        windowLayout.zIndex = 900;
                        windowLayout.interactable = true;
                        windowLayout.resizable = true;
                        retained.SetLayout(kWindow, windowLayout);

                        auto& row1 = retained.EnsureHStack(kRow1, kWindow, "Row 1");
                        LayoutStyle row1Layout = row1.layout;
                        row1Layout.mode = LayoutMode::HStack;
                        row1Layout.height = 42.0f;
                        row1Layout.fillX = true;
                        row1Layout.fillY = false;
                        row1Layout.spacing = 8.0f;
                        retained.SetLayout(kRow1, row1Layout);

                        auto& row2 = retained.EnsureHStack(kRow2, kWindow, "Row 2");
                        LayoutStyle row2Layout = row2.layout;
                        row2Layout.mode = LayoutMode::HStack;
                        row2Layout.height = 42.0f;
                        row2Layout.fillX = true;
                        row2Layout.fillY = false;
                        row2Layout.spacing = 8.0f;
                        retained.SetLayout(kRow2, row2Layout);

                        auto& row3 = retained.EnsureGrid(kRow3, kWindow, "Row 3");
                        LayoutStyle row3Layout = row3.layout;
                        row3Layout.mode = LayoutMode::Grid;
                        row3Layout.gridColumns = 2;
                        row3Layout.height = 120.0f;
                        row3Layout.fillX = true;
                        row3Layout.fillY = false;
                        row3Layout.spacing = 8.0f;
                        retained.SetLayout(kRow3, row3Layout);

                        retained.EnsureSearchInput(1100, kRow1, "Search");
                        retained.SetPlaceholder(1100, "type to filter assets...");

                        retained.EnsureButton(1101, kRow1, "Apply");
                        retained.EnsureSplitButton(1102, kRow1, "Presets");
                        retained.EnsureToggleSwitch(1103, kRow1, "Realtime");
                        retained.SetChecked(1103, true);

                        retained.EnsureSlider(1200, kRow2, "Exposure");
                        retained.SetRange(1200, -5.0f, 5.0f);
                        retained.SetValue(1200, 1.2f);

                        retained.EnsureRangeSlider(1201, kRow2, "Luminance");
                        retained.SetRange(1201, 0.0f, 10.0f);
                        retained.SetValue(1201, 1.5f);
                        retained.SetSecondaryValue(1201, 7.0f);

                        retained.EnsureProgressBar(1202, kRow2, "Streaming");
                        retained.SetRange(1202, 0.0f, 1.0f);
                        retained.SetValue(1202, 0.64f);

                        retained.EnsureCheckbox(1300, kRow3, "SSAO");
                        retained.SetChecked(1300, true);
                        retained.EnsureCheckbox(1301, kRow3, "SSR");
                        retained.SetChecked(1301, true);
                        retained.EnsureCheckbox(1302, kRow3, "Bloom");
                        retained.SetChecked(1302, true);
                        retained.EnsureCheckbox(1303, kRow3, "Vignette");
                        retained.SetChecked(1303, false);

                        auto& dataList = retained.EnsureVirtualList(kDataList, kWindow, "Frame Breakdown");
                        LayoutStyle dataLayout = dataList.layout;
                        dataLayout.fillX = true;
                        dataLayout.fillY = true;
                        dataLayout.height = 260.0f;
                        retained.SetLayout(kDataList, dataLayout);
                        retained.SetItems(kDataList,
                                          {"Renderer::BeginFrame 1.12ms",
                                           "ShadowPass 2.61ms",
                                           "GeometryPass 3.42ms",
                                           "PostProcess 1.33ms",
                                           "UI Composite 0.18ms",
                                           "GPU Frame 8.91ms"});

                        auto& footer = retained.EnsureHStack(kFooter, kWindow, "Footer");
                        LayoutStyle footerLayout = footer.layout;
                        footerLayout.mode = LayoutMode::HStack;
                        footerLayout.height = 38.0f;
                        footerLayout.fillX = true;
                        footerLayout.fillY = false;
                        footerLayout.spacing = 8.0f;
                        retained.SetLayout(kFooter, footerLayout);

                        retained.EnsureButton(1500, kFooter, "Save Layout");
                        retained.EnsureButton(1501, kFooter, "Load Layout");
                        retained.EnsureCommandPalette(1502, kFooter, "Command Palette");

                        AddOutput("ui.retained.demo: created retained component showcase");
                    });

    RegisterCommand("echo", "Print text to console", "echo <text>",
                    [this](const std::vector<std::string>& args) { AddOutput(Join(args, 0)); });

    RegisterCommand("set", "Set cvar value", "set <cvar> <value>",
                    [this](const std::vector<std::string>& args) {
                        if (args.size() < 2) {
                            AddOutput("usage: set <cvar> <value>");
                            return;
                        }

                        const std::string value = Join(args, 1);
                        if (!SetCVarFromString(args[0], value)) {
                            AddOutput("set: failed for cvar '" + args[0] + "'");
                            return;
                        }

                        AddOutput(args[0] + " = " + GetCVarAsString(args[0]));
                    });

    RegisterCommand("toggle", "Toggle bool/int cvar", "toggle <cvar>",
                    [this](const std::vector<std::string>& args) {
                        if (args.empty()) {
                            AddOutput("usage: toggle <cvar>");
                            return;
                        }

                        ConsoleVar* var = FindCVar(args[0]);
                        if (!var) {
                            AddOutput("toggle: unknown cvar '" + args[0] + "'");
                            return;
                        }

                        switch (var->type) {
                            case ConsoleVarType::Bool: {
                                var->value = !std::get<bool>(var->value);
                                break;
                            }
                            case ConsoleVarType::Int: {
                                var->value = (std::get<int>(var->value) == 0) ? 1 : 0;
                                break;
                            }
                            default:
                                AddOutput("toggle: only bool/int cvars are supported");
                                return;
                        }

                        if (var->onChanged) var->onChanged(*var);
                        AddOutput(var->name + " = " + CVarValueToString(*var));
                    });

    RegisterCommand("inc", "Increment numeric cvar", "inc <cvar> [amount]",
                    [this](const std::vector<std::string>& args) {
                        if (args.empty()) {
                            AddOutput("usage: inc <cvar> [amount]");
                            return;
                        }

                        ConsoleVar* var = FindCVar(args[0]);
                        if (!var) {
                            AddOutput("inc: unknown cvar '" + args[0] + "'");
                            return;
                        }

                        float amount = 1.0f;
                        if (args.size() > 1 && !ParseFloat(args[1], &amount)) {
                            AddOutput("inc: invalid amount '" + args[1] + "'");
                            return;
                        }

                        switch (var->type) {
                            case ConsoleVarType::Int:
                                var->value = std::get<int>(var->value) + static_cast<int>(amount);
                                break;
                            case ConsoleVarType::Float:
                                var->value = std::get<float>(var->value) + amount;
                                break;
                            default:
                                AddOutput("inc: only int/float cvars are supported");
                                return;
                        }

                        if (var->onChanged) var->onChanged(*var);
                        AddOutput(var->name + " = " + CVarValueToString(*var));
                    });

    RegisterCommand("dec", "Decrement numeric cvar", "dec <cvar> [amount]",
                    [this](const std::vector<std::string>& args) {
                        if (args.empty()) {
                            AddOutput("usage: dec <cvar> [amount]");
                            return;
                        }

                        ConsoleVar* var = FindCVar(args[0]);
                        if (!var) {
                            AddOutput("dec: unknown cvar '" + args[0] + "'");
                            return;
                        }

                        float amount = 1.0f;
                        if (args.size() > 1 && !ParseFloat(args[1], &amount)) {
                            AddOutput("dec: invalid amount '" + args[1] + "'");
                            return;
                        }

                        switch (var->type) {
                            case ConsoleVarType::Int:
                                var->value = std::get<int>(var->value) - static_cast<int>(amount);
                                break;
                            case ConsoleVarType::Float:
                                var->value = std::get<float>(var->value) - amount;
                                break;
                            default:
                                AddOutput("dec: only int/float cvars are supported");
                                return;
                        }

                        if (var->onChanged) var->onChanged(*var);
                        AddOutput(var->name + " = " + CVarValueToString(*var));
                    });

    RegisterCommand("reset", "Reset cvar to default", "reset <cvar>",
                    [this](const std::vector<std::string>& args) {
                        if (args.empty()) {
                            AddOutput("usage: reset <cvar>");
                            return;
                        }

                        ConsoleVar* var = FindCVar(args[0]);
                        if (!var) {
                            AddOutput("reset: unknown cvar '" + args[0] + "'");
                            return;
                        }

                        var->value = var->defaultValue;
                        if (var->onChanged) var->onChanged(*var);
                        AddOutput(var->name + " = " + CVarValueToString(*var));
                    });

    RegisterCommand("exec", "Execute commands from a cfg file", "exec <path>",
                    [this](const std::vector<std::string>& args) {
                        if (args.empty()) {
                            AddOutput("usage: exec <path>");
                            return;
                        }

                        const std::filesystem::path path = args[0];
                        std::ifstream               file(path);
                        if (!file.is_open()) {
                            AddOutput("Failed to open config file: " + path.string());
                            return;
                        }

                        AddOutput("Executing config: " + path.string());
                        std::string line;
                        while (std::getline(file, line)) {
                            if (line.empty()) continue;
                            Execute(line);
                        }
                    });

    RegisterCommand("writeconfig", "Save archived cvars and input bindings",
                    "writeconfig [cfgPath] [bindingsPath]",
                    [this](const std::vector<std::string>& args) {
                        std::filesystem::path cfgPath = defaultConsoleConfigPath_;
                        std::filesystem::path bindsPath = defaultInputBindingsPath_;

                        if (!args.empty()) cfgPath = args[0];
                        if (args.size() > 1) bindsPath = args[1];

                        const bool cfgOk   = SaveConfig(cfgPath);
                        const bool bindOk  = InputManager::Get().SaveBindings(bindsPath);

                        AddOutput(std::string("Saved console config: ") + (cfgOk ? "ok" : "failed") +
                                  " -> " + cfgPath.string());
                        AddOutput(std::string("Saved input bindings: ") + (bindOk ? "ok" : "failed") +
                                  " -> " + bindsPath.string());
                    });

    RegisterCommand("input.save", "Save input bindings JSON",
                    "input.save [path]",
                    [this](const std::vector<std::string>& args) {
                        const std::filesystem::path path =
                            args.empty() ? defaultInputBindingsPath_ : std::filesystem::path(args[0]);
                        bool ok = InputManager::Get().SaveBindings(path);
                        AddOutput(std::string("input.save: ") + (ok ? "ok" : "failed") + " -> " +
                                  path.string());
                    });

    RegisterCommand("input.load", "Load input bindings JSON",
                    "input.load [path]",
                    [this](const std::vector<std::string>& args) {
                        const std::filesystem::path path =
                            args.empty() ? defaultInputBindingsPath_ : std::filesystem::path(args[0]);
                        bool ok = InputManager::Get().LoadBindings(path);
                        EnsureDefaultBindings();
                        AddOutput(std::string("input.load: ") + (ok ? "ok" : "failed") + " -> " +
                                  path.string());
                    });

    RegisterCommand("input.reset", "Reset bindings to runtime defaults", "input.reset",
                    [this](const std::vector<std::string>& /*args*/) {
                        InputManager::Get().ResetBindings();
                        EnsureDefaultBindings();
                        AddOutput("input.reset: ok");
                    });

    RegisterCommand("input.list", "List action maps and bindings",
                    "input.list [map]",
                    [this](const std::vector<std::string>& args) {
                        const std::string targetMap = args.empty() ? "" : args[0];
                        const auto& maps            = InputManager::Get().GetActionMaps();

                        for (const auto& [mapName, map] : maps) {
                            if (!targetMap.empty() && mapName != targetMap) continue;
                            AddOutput("[Map] " + mapName + (map.enabled ? " (enabled)" : " (disabled)") +
                                      (map.consumeLowerPriority ? " [consume-lower]" : ""));
                            for (const auto& [actionName, action] : map.actions) {
                                AddOutput("  " + actionName + " [" +
                                          std::string(action.type == InputActionType::Button ? "button" : "axis") +
                                          "]");
                                for (const auto& binding : action.bindings) {
                                    std::string bindingText = "    - ";
                                    switch (binding.kind) {
                                        case InputBindingKind::Key:
                                            bindingText += "key";
                                            break;
                                        case InputBindingKind::MouseButton:
                                            bindingText += "mouse_button";
                                            break;
                                        case InputBindingKind::MouseAxisX:
                                            bindingText += "mouse_axis_x";
                                            break;
                                        case InputBindingKind::MouseAxisY:
                                            bindingText += "mouse_axis_y";
                                            break;
                                        case InputBindingKind::MouseScrollY:
                                            bindingText += "mouse_scroll_y";
                                            break;
                                        case InputBindingKind::GamepadButton:
                                            bindingText += "gamepad_button";
                                            break;
                                        case InputBindingKind::GamepadAxis:
                                            bindingText += "gamepad_axis";
                                            break;
                                    }
                                    switch (binding.kind) {
                                        case InputBindingKind::Key:
                                            bindingText += " " + InputManager::KeyToString(binding.key);
                                            break;
                                        case InputBindingKind::MouseButton:
                                            bindingText += " " +
                                                           InputManager::MouseButtonToString(binding.mouseButton);
                                            break;
                                        case InputBindingKind::GamepadButton:
                                            bindingText += " " +
                                                           InputManager::GamepadButtonToString(binding.gamepadButton);
                                            break;
                                        case InputBindingKind::GamepadAxis:
                                            bindingText += " " +
                                                           InputManager::GamepadAxisToString(binding.gamepadAxis);
                                            break;
                                        case InputBindingKind::MouseAxisX:
                                        case InputBindingKind::MouseAxisY:
                                        case InputBindingKind::MouseScrollY:
                                            break;
                                    }
                                    if (std::abs(binding.scale - 1.0f) > 1e-4f) {
                                        bindingText += " scale=" + std::to_string(binding.scale);
                                    }
                                    if (binding.invert) {
                                        bindingText += " invert=1";
                                    }
                                    AddOutput(bindingText);
                                }
                            }
                        }
                    });

    RegisterCommand("bind", "Bind keyboard key to button action", "bind <key> <action> [map]",
                    [this](const std::vector<std::string>& args) {
                        if (args.size() < 2) {
                            AddOutput("usage: bind <key> <action> [map]");
                            return;
                        }

                        KeyCode key = 0;
                        if (!InputManager::TryParseKey(args[0], &key)) {
                            AddOutput("Unknown key: " + args[0]);
                            return;
                        }

                        const std::string map = args.size() > 2 ? args[2] : "global";
                        InputManager::Get().CreateActionMap(map, true);
                        InputManager::Get().BindAction(map, args[1], key);

                        AddOutput("Bound key " + InputManager::KeyToString(key) + " to action " +
                                  args[1] + " in map " + map);
                    });

    RegisterCommand("bind_axis", "Bind keyboard/mouse key to axis action",
                    "bind_axis <key> <axisAction> [scale] [map]",
                    [this](const std::vector<std::string>& args) {
                        if (args.size() < 2) {
                            AddOutput("usage: bind_axis <key> <axisAction> [scale] [map]");
                            return;
                        }

                        KeyCode key = 0;
                        if (!InputManager::TryParseKey(args[0], &key)) {
                            AddOutput("Unknown key: " + args[0]);
                            return;
                        }

                        float       scale = 1.0f;
                        std::string map   = "global";
                        if (args.size() > 2) {
                            float maybeScale = 1.0f;
                            if (ParseFloat(args[2], &maybeScale)) {
                                scale = maybeScale;
                            } else {
                                map = args[2];
                            }
                        }
                        if (args.size() > 3) {
                            map = args[3];
                        }

                        InputManager::Get().CreateActionMap(map, true);
                        InputManager::Get().BindAxis(map, args[1], key, scale);

                        AddOutput("Bound key " + InputManager::KeyToString(key) + " to axis " +
                                  args[1] + " in map " + map);
                    });

    RegisterCommand("bind_mouse", "Bind mouse button to action",
                    "bind_mouse <button> <action> [map]",
                    [this](const std::vector<std::string>& args) {
                        if (args.size() < 2) {
                            AddOutput("usage: bind_mouse <button> <action> [map]");
                            return;
                        }

                        MouseButton button = Mouse::ButtonLeft;
                        if (!InputManager::TryParseMouseButton(args[0], &button)) {
                            AddOutput("Unknown mouse button: " + args[0]);
                            return;
                        }

                        const std::string map = args.size() > 2 ? args[2] : "global";
                        InputManager::Get().CreateActionMap(map, true);
                        InputManager::Get().BindActionMouse(map, args[1], button);

                        AddOutput("Bound mouse button " + InputManager::MouseButtonToString(button) +
                                  " to action " + args[1] + " in map " + map);
                    });

    RegisterCommand("bind_padbtn", "Bind gamepad button to action",
                    "bind_padbtn <button> <action> [map]",
                    [this](const std::vector<std::string>& args) {
                        if (args.size() < 2) {
                            AddOutput("usage: bind_padbtn <button> <action> [map]");
                            return;
                        }

                        GamepadButton button = Gamepad::A;
                        if (!InputManager::TryParseGamepadButton(args[0], &button)) {
                            AddOutput("Unknown gamepad button: " + args[0]);
                            return;
                        }

                        const std::string map = args.size() > 2 ? args[2] : "global";
                        InputManager::Get().CreateActionMap(map, true);
                        InputManager::Get().BindActionGamepad(map, args[1], button);

                        AddOutput("Bound gamepad button " + InputManager::GamepadButtonToString(button) +
                                  " to action " + args[1] + " in map " + map);
                    });

    RegisterCommand("bind_padaxis", "Bind gamepad axis to axis action",
                    "bind_padaxis <axis> <action> [scale] [invert] [map]",
                    [this](const std::vector<std::string>& args) {
                        if (args.size() < 2) {
                            AddOutput("usage: bind_padaxis <axis> <action> [scale] [invert] [map]");
                            return;
                        }

                        GamepadAxis axis = Gamepad::LeftX;
                        if (!InputManager::TryParseGamepadAxis(args[0], &axis)) {
                            AddOutput("Unknown gamepad axis: " + args[0]);
                            return;
                        }

                        float       scale  = 1.0f;
                        bool        invert = false;
                        std::string map    = "global";

                        if (args.size() > 2) {
                            float maybeScale = 1.0f;
                            if (ParseFloat(args[2], &maybeScale)) {
                                scale = maybeScale;
                            } else {
                                map = args[2];
                            }
                        }
                        if (args.size() > 3) {
                            bool maybeInvert = false;
                            if (ParseBool(args[3], &maybeInvert)) {
                                invert = maybeInvert;
                            } else {
                                map = args[3];
                            }
                        }
                        if (args.size() > 4) {
                            map = args[4];
                        }

                        InputManager::Get().CreateActionMap(map, true);
                        InputManager::Get().BindAxisGamepad(map, args[1], axis, scale, invert);

                        AddOutput("Bound gamepad axis " + InputManager::GamepadAxisToString(axis) +
                                  " to axis action " + args[1] + " in map " + map);
                    });

    RegisterCommand("unbind", "Unbind keyboard key in map", "unbind <key> [map]",
                    [this](const std::vector<std::string>& args) {
                        if (args.empty()) {
                            AddOutput("usage: unbind <key> [map]");
                            return;
                        }

                        KeyCode key = 0;
                        if (!InputManager::TryParseKey(args[0], &key)) {
                            AddOutput("Unknown key: " + args[0]);
                            return;
                        }

                        const std::string map = args.size() > 1 ? args[1] : "global";
                        bool              ok  = InputManager::Get().UnbindKey(map, key);
                        AddOutput(std::string("unbind: ") + (ok ? "ok" : "no matching binding"));
                    });

    RegisterCommand("unbind_mouse", "Unbind mouse button in map", "unbind_mouse <button> [map]",
                    [this](const std::vector<std::string>& args) {
                        if (args.empty()) {
                            AddOutput("usage: unbind_mouse <button> [map]");
                            return;
                        }

                        MouseButton button = Mouse::ButtonLeft;
                        if (!InputManager::TryParseMouseButton(args[0], &button)) {
                            AddOutput("Unknown mouse button: " + args[0]);
                            return;
                        }

                        const std::string map = args.size() > 1 ? args[1] : "global";
                        bool ok = InputManager::Get().UnbindMouseButton(map, button);
                        AddOutput(std::string("unbind_mouse: ") + (ok ? "ok" : "no matching binding"));
                    });

    RegisterCommand("unbind_padbtn", "Unbind gamepad button in map", "unbind_padbtn <button> [map]",
                    [this](const std::vector<std::string>& args) {
                        if (args.empty()) {
                            AddOutput("usage: unbind_padbtn <button> [map]");
                            return;
                        }

                        GamepadButton button = Gamepad::A;
                        if (!InputManager::TryParseGamepadButton(args[0], &button)) {
                            AddOutput("Unknown gamepad button: " + args[0]);
                            return;
                        }

                        const std::string map = args.size() > 1 ? args[1] : "global";
                        bool ok = InputManager::Get().UnbindGamepadButton(map, button);
                        AddOutput(std::string("unbind_padbtn: ") + (ok ? "ok" : "no matching binding"));
                    });

    RegisterCommand("unbind_action", "Remove action from map", "unbind_action <action> [map]",
                    [this](const std::vector<std::string>& args) {
                        if (args.empty()) {
                            AddOutput("usage: unbind_action <action> [map]");
                            return;
                        }

                        const std::string map = args.size() > 1 ? args[1] : "global";
                        bool              ok  = InputManager::Get().UnbindAction(map, args[0]);
                        ok = InputManager::Get().UnbindAxis(map, args[0]) || ok;
                        AddOutput(std::string("unbind_action: ") + (ok ? "ok" : "not found"));
                    });

    RegisterCommand("context.push", "Push action map context", "context.push <map>",
                    [this](const std::vector<std::string>& args) {
                        if (args.empty()) {
                            AddOutput("usage: context.push <map>");
                            return;
                        }

                        InputManager::Get().CreateActionMap(args[0], true);
                        bool ok = InputManager::Get().PushContext(args[0]);
                        AddOutput(std::string("context.push: ") + (ok ? "ok" : "failed"));
                    });

    RegisterCommand("context.pop", "Pop context (or named context)", "context.pop [map]",
                    [this](const std::vector<std::string>& args) {
                        bool ok = false;
                        if (args.empty()) {
                            ok = InputManager::Get().PopContext();
                        } else {
                            ok = InputManager::Get().PopContext(args[0]);
                        }
                        AddOutput(std::string("context.pop: ") + (ok ? "ok" : "failed"));
                    });

    RegisterCommand("context.list", "List active context stack", "context.list",
                    [this](const std::vector<std::string>& /*args*/) {
                        const auto& stack = InputManager::Get().GetContextStack();
                        AddOutput("Context stack (bottom -> top):");
                        for (const auto& ctx : stack) {
                            AddOutput("  " + ctx);
                        }
                    });

    RegisterCommand("context.clear", "Clear all contexts and restore defaults", "context.clear",
                    [this](const std::vector<std::string>& /*args*/) {
                        InputManager::Get().ClearContextStack();
                        EnsureDefaultBindings();
                        AddOutput("context.clear: ok");
                    });

    RegisterCommand("input.map_enable", "Enable or disable an action map",
                    "input.map_enable <map> <0|1>",
                    [this](const std::vector<std::string>& args) {
                        if (args.size() < 2) {
                            AddOutput("usage: input.map_enable <map> <0|1>");
                            return;
                        }

                        bool enabled = false;
                        if (!ParseBool(args[1], &enabled)) {
                            AddOutput("input.map_enable: invalid bool '" + args[1] + "'");
                            return;
                        }

                        InputManager::Get().CreateActionMap(args[0], true);
                        bool ok = InputManager::Get().SetActionMapEnabled(args[0], enabled);
                        AddOutput(std::string("input.map_enable: ") + (ok ? "ok" : "failed"));
                    });

    RegisterCommand("input.map_consume", "Set consume-lower-priority flag for a map",
                    "input.map_consume <map> <0|1>",
                    [this](const std::vector<std::string>& args) {
                        if (args.size() < 2) {
                            AddOutput("usage: input.map_consume <map> <0|1>");
                            return;
                        }

                        bool consume = false;
                        if (!ParseBool(args[1], &consume)) {
                            AddOutput("input.map_consume: invalid bool '" + args[1] + "'");
                            return;
                        }

                        InputManager::Get().CreateActionMap(args[0], true);
                        bool ok = InputManager::Get().SetActionMapConsumeLowerPriority(args[0], consume);
                        AddOutput(std::string("input.map_consume: ") + (ok ? "ok" : "failed"));
                    });

    RegisterCommand("toggleconsole", "Toggle console visibility", "toggleconsole",
                    [this](const std::vector<std::string>& /*args*/) { ToggleVisible(); });

    RegisterCommand("quit", "Close application", "quit",
                    [this](const std::vector<std::string>& /*args*/) { Application::Get().Close(); });
}

void ConsoleSystem::RegisterDefaultCVars() {
    const auto* settingsSystem = ServiceLocator::Get().GetRenderSettingsSystemPtr();

    const int  fpsDefault   = Application::Get().GetWindow().GetTargetFPS();
    const bool vsyncDefault = Application::Get().GetWindow().IsVSync();

    const bool postProcessDefault = settingsSystem ? settingsSystem->GetSettings().postProcessingEnabled : true;
    const bool shadowDefault      = settingsSystem ? settingsSystem->GetSettings().shadowingEnabled : true;
    const bool cullingDefault     = settingsSystem ? settingsSystem->GetSettings().frustumCullingEnabled : true;
    const bool frontFaceDefault   = settingsSystem ? settingsSystem->GetSettings().frontFaceWindingInverted : false;
    const bool ssaoDefault        = settingsSystem ? settingsSystem->GetSettings().aoOptions.enabled : true;
    const bool ssrDefault         = settingsSystem ? settingsSystem->GetSettings().ssrOptions.enabled : true;
    const bool bloomDefault       = settingsSystem ? settingsSystem->GetSettings().bloomOptions.enabled : true;
    const bool fogDefault         = settingsSystem ? settingsSystem->GetSettings().fogOptions.enabled : false;
    const bool vignetteDefault    = settingsSystem ? settingsSystem->GetSettings().vignetteOptions.enabled : false;
    const bool uiRetainDefault    = ui::NativeUiRenderer::Get().IsRetainedGeometryReuseEnabled();
    const bool uiFlipVDefault     = ui::NativeUiRenderer::Get().IsFlipUvV();

    const int aaDefault  = settingsSystem ? settingsSystem->GetSettings().antiAliasing : 1;
    const int hdrDefault = settingsSystem ? settingsSystem->GetSettings().hdrQuality : 2;

    RegisterIntCVar("fps_max", fpsDefault, "Frame cap. 0 = uncapped", true,
                    [](const ConsoleVar& var) {
                        int value = std::get<int>(var.value);
                        if (value < 0) value = 0;
                        Application::Get().GetWindow().SetTargetFPS(value);
                    });

    RegisterBoolCVar("r_vsync", vsyncDefault, "Swap interval preference", true,
                     [](const ConsoleVar& var) {
                         Application::Get().GetWindow().SetVSync(std::get<bool>(var.value));
                     });

    RegisterBoolCVar("r_postprocess", postProcessDefault, "Enable post-processing", true,
                     [](const ConsoleVar& var) {
                         auto* settings = ServiceLocator::Get().GetRenderSettingsSystemPtr();
                         if (!settings) return;
                         settings->GetMutableSettings().postProcessingEnabled = std::get<bool>(var.value);
                         settings->MarkDirty();
                         settings->Apply();
                     });

    RegisterBoolCVar("r_shadows", shadowDefault, "Enable view shadowing", true,
                     [](const ConsoleVar& var) {
                         auto* settings = ServiceLocator::Get().GetRenderSettingsSystemPtr();
                         if (!settings) return;
                         settings->GetMutableSettings().shadowingEnabled = std::get<bool>(var.value);
                         settings->MarkDirty();
                         settings->Apply();
                     });

    RegisterBoolCVar("r_frustum_culling", cullingDefault, "Enable frustum culling", true,
                     [](const ConsoleVar& var) {
                         auto* settings = ServiceLocator::Get().GetRenderSettingsSystemPtr();
                         if (!settings) return;
                         settings->GetMutableSettings().frustumCullingEnabled = std::get<bool>(var.value);
                         settings->MarkDirty();
                         settings->Apply();
                     });

    RegisterBoolCVar("r_frontface_inverted", frontFaceDefault,
                     "Invert front-face winding", true,
                     [](const ConsoleVar& var) {
                         auto* settings = ServiceLocator::Get().GetRenderSettingsSystemPtr();
                         if (!settings) return;
                         settings->GetMutableSettings().frontFaceWindingInverted = std::get<bool>(var.value);
                         settings->MarkDirty();
                         settings->Apply();
                     });

    RegisterBoolCVar("r_ssao", ssaoDefault, "Enable SSAO", true,
                     [](const ConsoleVar& var) {
                         auto* settings = ServiceLocator::Get().GetRenderSettingsSystemPtr();
                         if (!settings) return;
                         settings->GetMutableSettings().aoOptions.enabled = std::get<bool>(var.value);
                         settings->MarkDirty();
                         settings->Apply();
                     });

    RegisterBoolCVar("r_ssr", ssrDefault, "Enable SSR", true,
                     [](const ConsoleVar& var) {
                         auto* settings = ServiceLocator::Get().GetRenderSettingsSystemPtr();
                         if (!settings) return;
                         settings->GetMutableSettings().ssrOptions.enabled = std::get<bool>(var.value);
                         settings->MarkDirty();
                         settings->Apply();
                     });

    RegisterBoolCVar("r_bloom", bloomDefault, "Enable bloom", true,
                     [](const ConsoleVar& var) {
                         auto* settings = ServiceLocator::Get().GetRenderSettingsSystemPtr();
                         if (!settings) return;
                         settings->GetMutableSettings().bloomOptions.enabled = std::get<bool>(var.value);
                         settings->MarkDirty();
                         settings->Apply();
                     });

    RegisterBoolCVar("r_fog", fogDefault, "Enable fog", true,
                     [](const ConsoleVar& var) {
                         auto* settings = ServiceLocator::Get().GetRenderSettingsSystemPtr();
                         if (!settings) return;
                         settings->GetMutableSettings().fogOptions.enabled = std::get<bool>(var.value);
                         settings->MarkDirty();
                         settings->Apply();
                     });

    RegisterBoolCVar("r_vignette", vignetteDefault, "Enable vignette", true,
                     [](const ConsoleVar& var) {
                         auto* settings = ServiceLocator::Get().GetRenderSettingsSystemPtr();
                         if (!settings) return;
                         settings->GetMutableSettings().vignetteOptions.enabled = std::get<bool>(var.value);
                         settings->MarkDirty();
                         settings->Apply();
                     });

    RegisterIntCVar("r_aa", aaDefault, "Post AA mode (0 none, 1 FXAA)", true,
                    [](const ConsoleVar& var) {
                        auto* settings = ServiceLocator::Get().GetRenderSettingsSystemPtr();
                        if (!settings) return;
                        int value = std::clamp(std::get<int>(var.value), 0, 1);
                        settings->GetMutableSettings().antiAliasing = value;
                        settings->MarkDirty();
                        settings->Apply();
                    });

    RegisterIntCVar("r_hdr_quality", hdrDefault,
                    "HDR buffer quality (0 low, 1 medium, 2 high, 3 ultra)", true,
                    [](const ConsoleVar& var) {
                        auto* settings = ServiceLocator::Get().GetRenderSettingsSystemPtr();
                        if (!settings) return;
                        int value = std::clamp(std::get<int>(var.value), 0, 3);
                        settings->GetMutableSettings().hdrQuality = value;
                        settings->MarkDirty();
                        settings->Apply();
                    });

    RegisterBoolCVar("ui_retain_cache", uiRetainDefault,
                     "Reuse retained native UI geometry between frames", true,
                     [](const ConsoleVar& var) {
                         ui::NativeUiRenderer::Get().SetRetainedGeometryReuseEnabled(
                             std::get<bool>(var.value));
                     });

    RegisterBoolCVar("ui_flip_uv_v", uiFlipVDefault,
                     "Flip native UI atlas V coordinate (debug compatibility toggle)", true,
                     [](const ConsoleVar& var) {
                         ui::NativeUiRenderer::Get().SetFlipUvV(std::get<bool>(var.value));
                     });
}

void ConsoleSystem::AttachLogSink() {
    if (logSink_) return;

    auto logger = Logger();
    if (!logger) return;

    logSink_ = std::make_shared<ConsoleSpdlogSink>(this);
    auto& sinks = logger->sinks();
    sinks.push_back(logSink_);
}

void ConsoleSystem::DetachLogSink() {
    if (!logSink_) return;

    auto logger = Logger();
    if (logger) {
        auto& sinks = logger->sinks();
        sinks.erase(std::remove(sinks.begin(), sinks.end(), logSink_), sinks.end());
    }

    logSink_.reset();
}

void ConsoleSystem::EnsureDefaultBindings() {
    auto& input = InputManager::Get();

    input.CreateActionMap(engineMapName_, true, false);
    input.SetActionMapEnabled(engineMapName_, true);
    input.SetActionMapConsumeLowerPriority(engineMapName_, false);

    input.CreateActionMap(engineConsoleMapName_, false, true);
    input.SetActionMapEnabled(engineConsoleMapName_, false);
    input.SetActionMapConsumeLowerPriority(engineConsoleMapName_, true);

    if (!input.HasAction(engineMapName_, "engine_toggle_console")) {
        input.BindAction(engineMapName_, "engine_toggle_console", Key::GraveAccent);
    }
    if (!input.HasAction(engineConsoleMapName_, "engine_toggle_console")) {
        input.BindAction(engineConsoleMapName_, "engine_toggle_console", Key::GraveAccent);
    }

    SyncConsoleContextState();
}

void ConsoleSystem::SyncConsoleContextState() {
    auto& input = InputManager::Get();

    input.CreateActionMap(engineMapName_, true, false);
    input.SetActionMapEnabled(engineMapName_, true);
    input.SetActionMapConsumeLowerPriority(engineMapName_, false);
    input.PushContext(engineMapName_);

    input.CreateActionMap(engineConsoleMapName_, false, true);
    input.SetActionMapConsumeLowerPriority(engineConsoleMapName_, true);
    input.PushContext(engineConsoleMapName_);
    input.SetActionMapEnabled(engineConsoleMapName_, visible_);
}

}  // namespace se
