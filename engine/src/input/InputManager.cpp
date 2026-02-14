#include "engine/input/InputManager.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include "engine/Application.h"
#include "engine/Log.h"
#include "engine/input/GamepadManager.h"

namespace se {

namespace {

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool FloatEqual(float a, float b, float eps = 1e-5f) {
    return std::abs(a - b) <= eps;
}

bool BindingEquals(const InputBinding& a, const InputBinding& b) {
    return a.kind == b.kind && a.key == b.key && a.mouseButton == b.mouseButton &&
           a.gamepadButton == b.gamepadButton && a.gamepadAxis == b.gamepadAxis &&
           FloatEqual(a.scale, b.scale) && a.invert == b.invert;
}

const char* ActionTypeToString(InputActionType type) {
    return type == InputActionType::Button ? "button" : "axis";
}

bool TryParseActionType(const std::string& token, InputActionType* outType) {
    if (!outType) return false;
    const std::string lower = ToLower(token);
    if (lower == "button") {
        *outType = InputActionType::Button;
        return true;
    }
    if (lower == "axis") {
        *outType = InputActionType::Axis;
        return true;
    }
    return false;
}

const char* BindingKindToString(InputBindingKind kind) {
    switch (kind) {
        case InputBindingKind::Key:
            return "key";
        case InputBindingKind::MouseButton:
            return "mouse_button";
        case InputBindingKind::MouseAxisX:
            return "mouse_axis_x";
        case InputBindingKind::MouseAxisY:
            return "mouse_axis_y";
        case InputBindingKind::MouseScrollY:
            return "mouse_scroll_y";
        case InputBindingKind::GamepadButton:
            return "gamepad_button";
        case InputBindingKind::GamepadAxis:
            return "gamepad_axis";
    }
    return "key";
}

bool TryParseBindingKind(const std::string& token, InputBindingKind* outKind) {
    if (!outKind) return false;
    const std::string lower = ToLower(token);
    if (lower == "key") {
        *outKind = InputBindingKind::Key;
        return true;
    }
    if (lower == "mouse_button") {
        *outKind = InputBindingKind::MouseButton;
        return true;
    }
    if (lower == "mouse_axis_x") {
        *outKind = InputBindingKind::MouseAxisX;
        return true;
    }
    if (lower == "mouse_axis_y") {
        *outKind = InputBindingKind::MouseAxisY;
        return true;
    }
    if (lower == "mouse_scroll_y") {
        *outKind = InputBindingKind::MouseScrollY;
        return true;
    }
    if (lower == "gamepad_button") {
        *outKind = InputBindingKind::GamepadButton;
        return true;
    }
    if (lower == "gamepad_axis") {
        *outKind = InputBindingKind::GamepadAxis;
        return true;
    }
    return false;
}

}  // namespace

void InputManager::Init(EventBus* eventBus) {
    eventBus_ = eventBus;

    CreateActionMap(defaultMapName_);
    PushContext(defaultMapName_);

    GamepadManager::Get().Init(eventBus);

    SE_LOG_INFO("InputManager initialized");
}

void InputManager::Shutdown() {
    GamepadManager::Get().Shutdown();
    actionMaps_.clear();
    contextStack_.clear();
    textInputQueue_.clear();
    ResetAllInputStates();
    SE_LOG_INFO("InputManager shutdown");
}

void InputManager::Update() {
    ResetPerFrameTransitions();
    mouseDelta_  = {0.0f, 0.0f};
    scrollDelta_ = 0.0f;
    // NOTE: textInputQueue_ is NOT cleared here.  It is drained by
    // ConsumeTextInput() when the consumer layer runs.  If a Filament frame is
    // skipped (BeginFrame() returns false), layers don't execute and any queued
    // text survives to the next frame instead of being silently dropped.

    GamepadManager::Get().Update();
}

void InputManager::SetCursorMode(CursorMode mode) {
    auto&        app    = Application::Get();
    WindowHandle window = app.GetWindow().GetNativeWindow();

    int glfwMode = GLFW_CURSOR_NORMAL;
    switch (mode) {
        case CursorMode::Normal:
            glfwMode = GLFW_CURSOR_NORMAL;
            break;
        case CursorMode::Hidden:
            glfwMode = GLFW_CURSOR_HIDDEN;
            break;
        case CursorMode::Locked:
            glfwMode = GLFW_CURSOR_DISABLED;
            break;
    }

    cursorMode_ = mode;
    firstMouse_ = true;

    glfwSetInputMode(window, GLFW_CURSOR, glfwMode);
}

bool InputManager::CreateActionMap(const std::string& mapName, bool enabled,
                                   bool consumeLowerPriority) {
    if (mapName.empty()) return false;
    if (actionMaps_.contains(mapName)) return false;

    InputActionMap map;
    map.name                 = mapName;
    map.enabled              = enabled;
    map.consumeLowerPriority = consumeLowerPriority;
    actionMaps_.emplace(mapName, std::move(map));
    return true;
}

bool InputManager::RemoveActionMap(const std::string& mapName) {
    if (mapName.empty()) return false;
    if (mapName == defaultMapName_) return false;

    auto erased = actionMaps_.erase(mapName);
    if (erased == 0) return false;

    contextStack_.erase(
        std::remove(contextStack_.begin(), contextStack_.end(), mapName),
        contextStack_.end());

    return true;
}

bool InputManager::HasActionMap(const std::string& mapName) const {
    return actionMaps_.contains(mapName);
}

bool InputManager::SetActionMapEnabled(const std::string& mapName, bool enabled) {
    auto it = actionMaps_.find(mapName);
    if (it == actionMaps_.end()) return false;
    it->second.enabled = enabled;
    return true;
}

bool InputManager::IsActionMapEnabled(const std::string& mapName) const {
    auto it = actionMaps_.find(mapName);
    if (it == actionMaps_.end()) return false;
    return it->second.enabled;
}

bool InputManager::SetActionMapConsumeLowerPriority(const std::string& mapName,
                                                    bool consumeLowerPriority) {
    auto it = actionMaps_.find(mapName);
    if (it == actionMaps_.end()) return false;
    it->second.consumeLowerPriority = consumeLowerPriority;
    return true;
}

bool InputManager::DoesActionMapConsumeLowerPriority(const std::string& mapName) const {
    auto it = actionMaps_.find(mapName);
    if (it == actionMaps_.end()) return false;
    return it->second.consumeLowerPriority;
}

bool InputManager::HasAction(const std::string& mapName, const std::string& actionName) const {
    auto mapIt = actionMaps_.find(mapName);
    if (mapIt == actionMaps_.end()) return false;
    return mapIt->second.actions.contains(actionName);
}

bool InputManager::PushContext(const std::string& mapName) {
    if (!actionMaps_.contains(mapName)) return false;

    contextStack_.erase(
        std::remove(contextStack_.begin(), contextStack_.end(), mapName),
        contextStack_.end());

    contextStack_.push_back(mapName);
    return true;
}

bool InputManager::PopContext() {
    if (contextStack_.empty()) return false;
    contextStack_.pop_back();
    return true;
}

bool InputManager::PopContext(const std::string& mapName) {
    auto it = std::find(contextStack_.rbegin(), contextStack_.rend(), mapName);
    if (it == contextStack_.rend()) return false;
    contextStack_.erase(std::next(it).base());
    return true;
}

void InputManager::ClearContextStack() {
    contextStack_.clear();
}

bool InputManager::BindAction(const std::string& mapName, const std::string& actionName,
                              KeyCode key) {
    InputBinding binding;
    binding.kind = InputBindingKind::Key;
    binding.key  = key;
    return AddBinding(mapName, actionName, InputActionType::Button, binding);
}

bool InputManager::BindActionMouse(const std::string& mapName, const std::string& actionName,
                                   MouseButton button) {
    InputBinding binding;
    binding.kind        = InputBindingKind::MouseButton;
    binding.mouseButton = button;
    return AddBinding(mapName, actionName, InputActionType::Button, binding);
}

bool InputManager::BindActionGamepad(const std::string& mapName, const std::string& actionName,
                                     GamepadButton button) {
    InputBinding binding;
    binding.kind          = InputBindingKind::GamepadButton;
    binding.gamepadButton = button;
    return AddBinding(mapName, actionName, InputActionType::Button, binding);
}

bool InputManager::BindAxis(const std::string& mapName, const std::string& axisName, KeyCode key,
                            float scale) {
    InputBinding binding;

    InputBindingKind axisKind = InputBindingKind::Key;
    if (IsMouseAxisKey(key, &axisKind)) {
        binding.kind = axisKind;
    } else {
        binding.kind = InputBindingKind::Key;
        binding.key  = key;
    }
    binding.scale = scale;

    return AddBinding(mapName, axisName, InputActionType::Axis, binding);
}

bool InputManager::BindAxisMouse(const std::string& mapName, const std::string& axisName,
                                 InputBindingKind axisKind, float scale) {
    if (axisKind != InputBindingKind::MouseAxisX && axisKind != InputBindingKind::MouseAxisY &&
        axisKind != InputBindingKind::MouseScrollY) {
        return false;
    }

    InputBinding binding;
    binding.kind  = axisKind;
    binding.scale = scale;

    return AddBinding(mapName, axisName, InputActionType::Axis, binding);
}

bool InputManager::BindAxisGamepad(const std::string& mapName, const std::string& axisName,
                                   GamepadAxis axis, float scale, bool invert) {
    InputBinding binding;
    binding.kind        = InputBindingKind::GamepadAxis;
    binding.gamepadAxis = axis;
    binding.scale       = scale;
    binding.invert      = invert;
    return AddBinding(mapName, axisName, InputActionType::Axis, binding);
}

bool InputManager::UnbindAction(const std::string& mapName, const std::string& actionName) {
    auto mapIt = actionMaps_.find(mapName);
    if (mapIt == actionMaps_.end()) return false;

    auto it = mapIt->second.actions.find(actionName);
    if (it == mapIt->second.actions.end()) return false;
    if (it->second.type != InputActionType::Button) return false;

    mapIt->second.actions.erase(it);
    return true;
}

bool InputManager::UnbindAxis(const std::string& mapName, const std::string& axisName) {
    auto mapIt = actionMaps_.find(mapName);
    if (mapIt == actionMaps_.end()) return false;

    auto it = mapIt->second.actions.find(axisName);
    if (it == mapIt->second.actions.end()) return false;
    if (it->second.type != InputActionType::Axis) return false;

    mapIt->second.actions.erase(it);
    return true;
}

bool InputManager::UnbindKey(const std::string& mapName, KeyCode key) {
    auto mapIt = actionMaps_.find(mapName);
    if (mapIt == actionMaps_.end()) return false;

    bool changed = false;
    for (auto actionIt = mapIt->second.actions.begin(); actionIt != mapIt->second.actions.end();) {
        auto& bindings = actionIt->second.bindings;
        const auto beforeSize = bindings.size();

        bindings.erase(std::remove_if(bindings.begin(), bindings.end(), [key](const InputBinding& binding) {
                           return binding.kind == InputBindingKind::Key && binding.key == key;
                       }),
                       bindings.end());

        changed = changed || (beforeSize != bindings.size());

        if (bindings.empty()) {
            actionIt = mapIt->second.actions.erase(actionIt);
        } else {
            ++actionIt;
        }
    }

    return changed;
}

bool InputManager::UnbindMouseButton(const std::string& mapName, MouseButton button) {
    auto mapIt = actionMaps_.find(mapName);
    if (mapIt == actionMaps_.end()) return false;

    bool changed = false;
    for (auto actionIt = mapIt->second.actions.begin(); actionIt != mapIt->second.actions.end();) {
        auto& bindings = actionIt->second.bindings;
        const auto beforeSize = bindings.size();

        bindings.erase(
            std::remove_if(bindings.begin(), bindings.end(), [button](const InputBinding& binding) {
                return binding.kind == InputBindingKind::MouseButton &&
                       binding.mouseButton == button;
            }),
            bindings.end());

        changed = changed || (beforeSize != bindings.size());

        if (bindings.empty()) {
            actionIt = mapIt->second.actions.erase(actionIt);
        } else {
            ++actionIt;
        }
    }

    return changed;
}

bool InputManager::UnbindGamepadButton(const std::string& mapName, GamepadButton button) {
    auto mapIt = actionMaps_.find(mapName);
    if (mapIt == actionMaps_.end()) return false;

    bool changed = false;
    for (auto actionIt = mapIt->second.actions.begin(); actionIt != mapIt->second.actions.end();) {
        auto& bindings = actionIt->second.bindings;
        const auto beforeSize = bindings.size();

        bindings.erase(
            std::remove_if(bindings.begin(), bindings.end(), [button](const InputBinding& binding) {
                return binding.kind == InputBindingKind::GamepadButton &&
                       binding.gamepadButton == button;
            }),
            bindings.end());

        changed = changed || (beforeSize != bindings.size());

        if (bindings.empty()) {
            actionIt = mapIt->second.actions.erase(actionIt);
        } else {
            ++actionIt;
        }
    }

    return changed;
}

void InputManager::ResetBindings() {
    actionMaps_.clear();
    contextStack_.clear();

    CreateActionMap(defaultMapName_);
    PushContext(defaultMapName_);
}

void InputManager::BindAction(const std::string& name, KeyCode key) {
    BindAction(defaultMapName_, name, key);
}

void InputManager::BindAxis(const std::string& name, KeyCode key, float scale) {
    BindAxis(defaultMapName_, name, key, scale);
}

void InputManager::UnbindAction(const std::string& name) {
    for (auto& [mapName, map] : actionMaps_) {
        map.actions.erase(name);
    }
}

void InputManager::UnbindAxis(const std::string& name) {
    for (auto& [mapName, map] : actionMaps_) {
        map.actions.erase(name);
    }
}

void InputManager::BindGamepadAction(const std::string& name, GamepadButton button) {
    BindActionGamepad(defaultMapName_, name, button);
}

void InputManager::BindGamepadAxis(const std::string& name, GamepadAxis axis, float scale,
                                   bool invert) {
    BindAxisGamepad(defaultMapName_, name, axis, scale, invert);
}

void InputManager::UnbindGamepadAction(const std::string& name) {
    UnbindAction(name);
}

void InputManager::UnbindGamepadAxis(const std::string& name) {
    UnbindAxis(name);
}

bool InputManager::IsActionPressed(const std::string& name) const {
    const auto* action = FindAction(name);
    if (!action) return false;

    if (action->type == InputActionType::Axis) {
        return std::abs(GetAxis(name)) > 1e-4f;
    }

    for (const auto& binding : action->bindings) {
        if (EvaluateButtonBindingDown(binding)) return true;
    }
    return false;
}

bool InputManager::IsActionJustPressed(const std::string& name) const {
    const auto* action = FindAction(name);
    if (!action || action->type != InputActionType::Button) return false;

    for (const auto& binding : action->bindings) {
        if (EvaluateButtonBindingJustPressed(binding)) return true;
    }
    return false;
}

bool InputManager::IsActionJustReleased(const std::string& name) const {
    const auto* action = FindAction(name);
    if (!action || action->type != InputActionType::Button) return false;

    for (const auto& binding : action->bindings) {
        if (EvaluateButtonBindingJustReleased(binding)) return true;
    }
    return false;
}

float InputManager::GetAxis(const std::string& name) const {
    const auto* action = FindAction(name);
    if (!action) return 0.0f;

    float value = 0.0f;
    for (const auto& binding : action->bindings) {
        if (action->type == InputActionType::Axis) {
            value += EvaluateAxisBinding(binding);
        } else {
            value += EvaluateButtonBindingDown(binding) ? binding.scale : 0.0f;
        }
    }

    return value;
}

bool InputManager::IsKeyDown(KeyCode key) const {
    auto it = keyStates_.find(key);
    return it != keyStates_.end() && it->second.isDown;
}

bool InputManager::IsKeyJustPressed(KeyCode key) const {
    auto it = keyStates_.find(key);
    return it != keyStates_.end() && it->second.justPressed;
}

bool InputManager::IsKeyJustReleased(KeyCode key) const {
    auto it = keyStates_.find(key);
    return it != keyStates_.end() && it->second.justReleased;
}

bool InputManager::IsMouseButtonDown(MouseButton button) const {
    auto it = mouseButtonStates_.find(button);
    return it != mouseButtonStates_.end() && it->second.isDown;
}

bool InputManager::IsMouseButtonJustPressed(MouseButton button) const {
    auto it = mouseButtonStates_.find(button);
    return it != mouseButtonStates_.end() && it->second.justPressed;
}

bool InputManager::IsMouseButtonJustReleased(MouseButton button) const {
    auto it = mouseButtonStates_.find(button);
    return it != mouseButtonStates_.end() && it->second.justReleased;
}

Vector2 InputManager::GetMousePosition() const {
    return mousePosition_;
}

Vector2 InputManager::GetMouseDelta() const {
    return mouseDelta_;
}

bool InputManager::IsGamepadButtonDown(GamepadButton button) const {
    return GamepadManager::Get().IsButtonDown(button);
}

bool InputManager::IsGamepadButtonPressed(GamepadButton button) const {
    return GamepadManager::Get().IsButtonPressed(button);
}

bool InputManager::IsGamepadButtonReleased(GamepadButton button) const {
    return GamepadManager::Get().IsButtonReleased(button);
}

float InputManager::GetGamepadAxis(GamepadAxis axis) const {
    return GamepadManager::Get().GetAxis(axis);
}

bool InputManager::IsGamepadConnected(GamepadId id) const {
    return GamepadManager::Get().IsConnected(id);
}

bool InputManager::SaveBindings(const std::filesystem::path& path) const {
    try {
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }

        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);

        writer.StartObject();
        writer.Key("version");
        writer.Int(2);

        writer.Key("contexts");
        writer.StartArray();
        for (const auto& context : contextStack_) {
            writer.String(context.c_str());
        }
        writer.EndArray();

        writer.Key("maps");
        writer.StartArray();

        std::vector<std::string> mapNames;
        mapNames.reserve(actionMaps_.size());
        for (const auto& [name, map] : actionMaps_) {
            mapNames.push_back(name);
        }
        std::sort(mapNames.begin(), mapNames.end());

        for (const auto& mapName : mapNames) {
            const auto& map = actionMaps_.at(mapName);

            writer.StartObject();
            writer.Key("name");
            writer.String(map.name.c_str());
            writer.Key("enabled");
            writer.Bool(map.enabled);
            writer.Key("consumeLowerPriority");
            writer.Bool(map.consumeLowerPriority);

            writer.Key("actions");
            writer.StartArray();

            std::vector<std::string> actionNames;
            actionNames.reserve(map.actions.size());
            for (const auto& [actionName, action] : map.actions) {
                actionNames.push_back(actionName);
            }
            std::sort(actionNames.begin(), actionNames.end());

            for (const auto& actionName : actionNames) {
                const auto& action = map.actions.at(actionName);

                writer.StartObject();
                writer.Key("name");
                writer.String(actionName.c_str());
                writer.Key("type");
                writer.String(ActionTypeToString(action.type));

                writer.Key("bindings");
                writer.StartArray();
                for (const auto& binding : action.bindings) {
                    writer.StartObject();
                    writer.Key("kind");
                    writer.String(BindingKindToString(binding.kind));

                    switch (binding.kind) {
                        case InputBindingKind::Key:
                            writer.Key("key");
                            writer.String(KeyToString(binding.key).c_str());
                            break;
                        case InputBindingKind::MouseButton:
                            writer.Key("button");
                            writer.String(MouseButtonToString(binding.mouseButton).c_str());
                            break;
                        case InputBindingKind::GamepadButton:
                            writer.Key("button");
                            writer.String(GamepadButtonToString(binding.gamepadButton).c_str());
                            break;
                        case InputBindingKind::GamepadAxis:
                            writer.Key("axis");
                            writer.String(GamepadAxisToString(binding.gamepadAxis).c_str());
                            writer.Key("invert");
                            writer.Bool(binding.invert);
                            break;
                        case InputBindingKind::MouseAxisX:
                        case InputBindingKind::MouseAxisY:
                        case InputBindingKind::MouseScrollY:
                            break;
                    }

                    writer.Key("scale");
                    writer.Double(binding.scale);

                    writer.EndObject();
                }
                writer.EndArray();

                writer.EndObject();
            }

            writer.EndArray();
            writer.EndObject();
        }

        writer.EndArray();
        writer.EndObject();

        std::ofstream output(path, std::ios::binary);
        if (!output.is_open()) {
            SE_LOG_ERROR("Failed to open input bindings file for writing: {}", path.string());
            return false;
        }

        output << buffer.GetString();
        return true;
    } catch (const std::exception& e) {
        SE_LOG_ERROR("Failed to save input bindings: {}", e.what());
        return false;
    }
}

bool InputManager::LoadBindings(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        SE_LOG_WARN("Input bindings file not found: {}", path.string());
        return false;
    }

    std::ostringstream stream;
    stream << input.rdbuf();
    const std::string json = stream.str();

    rapidjson::Document document;
    document.Parse(json.c_str());
    if (document.HasParseError()) {
        SE_LOG_ERROR("Failed parsing input bindings JSON at offset {}: {}",
                     document.GetErrorOffset(),
                     rapidjson::GetParseError_En(document.GetParseError()));
        return false;
    }

    if (!document.IsObject()) {
        SE_LOG_ERROR("Invalid input bindings JSON root");
        return false;
    }

    ResetBindings();

    if (document.HasMember("maps") && document["maps"].IsArray()) {
        const auto& maps = document["maps"].GetArray();

        for (const auto& mapValue : maps) {
            if (!mapValue.IsObject() || !mapValue.HasMember("name") || !mapValue["name"].IsString()) {
                continue;
            }

            const std::string mapName = mapValue["name"].GetString();
            if (!actionMaps_.contains(mapName)) {
                CreateActionMap(mapName, true);
            }

            bool enabled = true;
            if (mapValue.HasMember("enabled") && mapValue["enabled"].IsBool()) {
                enabled = mapValue["enabled"].GetBool();
            }
            SetActionMapEnabled(mapName, enabled);

            bool consumeLowerPriority = false;
            if (mapValue.HasMember("consumeLowerPriority") &&
                mapValue["consumeLowerPriority"].IsBool()) {
                consumeLowerPriority = mapValue["consumeLowerPriority"].GetBool();
            }
            SetActionMapConsumeLowerPriority(mapName, consumeLowerPriority);

            if (!mapValue.HasMember("actions") || !mapValue["actions"].IsArray()) {
                continue;
            }

            const auto& actions = mapValue["actions"].GetArray();
            for (const auto& actionValue : actions) {
                if (!actionValue.IsObject() || !actionValue.HasMember("name") ||
                    !actionValue["name"].IsString()) {
                    continue;
                }

                const std::string actionName = actionValue["name"].GetString();

                InputActionType actionType = InputActionType::Button;
                if (actionValue.HasMember("type") && actionValue["type"].IsString()) {
                    TryParseActionType(actionValue["type"].GetString(), &actionType);
                }

                if (!actionValue.HasMember("bindings") || !actionValue["bindings"].IsArray()) {
                    continue;
                }

                const auto& bindings = actionValue["bindings"].GetArray();
                for (const auto& bindingValue : bindings) {
                    if (!bindingValue.IsObject() || !bindingValue.HasMember("kind") ||
                        !bindingValue["kind"].IsString()) {
                        continue;
                    }

                    InputBindingKind kind;
                    if (!TryParseBindingKind(bindingValue["kind"].GetString(), &kind)) {
                        continue;
                    }

                    InputBinding binding;
                    binding.kind = kind;

                    if (bindingValue.HasMember("scale") && bindingValue["scale"].IsNumber()) {
                        binding.scale = static_cast<float>(bindingValue["scale"].GetDouble());
                    }

                    if (bindingValue.HasMember("invert") && bindingValue["invert"].IsBool()) {
                        binding.invert = bindingValue["invert"].GetBool();
                    }

                    switch (kind) {
                        case InputBindingKind::Key: {
                            if (!bindingValue.HasMember("key") || !bindingValue["key"].IsString()) {
                                continue;
                            }
                            KeyCode key = 0;
                            if (!TryParseKey(bindingValue["key"].GetString(), &key)) {
                                continue;
                            }
                            binding.key = key;
                            break;
                        }
                        case InputBindingKind::MouseButton: {
                            if (!bindingValue.HasMember("button") ||
                                !bindingValue["button"].IsString()) {
                                continue;
                            }
                            MouseButton button = Mouse::ButtonLeft;
                            if (!TryParseMouseButton(bindingValue["button"].GetString(),
                                                     &button)) {
                                continue;
                            }
                            binding.mouseButton = button;
                            break;
                        }
                        case InputBindingKind::GamepadButton: {
                            if (!bindingValue.HasMember("button") ||
                                !bindingValue["button"].IsString()) {
                                continue;
                            }
                            GamepadButton button = Gamepad::A;
                            if (!TryParseGamepadButton(bindingValue["button"].GetString(),
                                                       &button)) {
                                continue;
                            }
                            binding.gamepadButton = button;
                            break;
                        }
                        case InputBindingKind::GamepadAxis: {
                            if (!bindingValue.HasMember("axis") || !bindingValue["axis"].IsString()) {
                                continue;
                            }
                            GamepadAxis axis = Gamepad::LeftX;
                            if (!TryParseGamepadAxis(bindingValue["axis"].GetString(), &axis)) {
                                continue;
                            }
                            binding.gamepadAxis = axis;
                            break;
                        }
                        case InputBindingKind::MouseAxisX:
                        case InputBindingKind::MouseAxisY:
                        case InputBindingKind::MouseScrollY:
                            break;
                    }

                    AddBinding(mapName, actionName, actionType, binding);
                }
            }
        }
    }

    contextStack_.clear();
    if (document.HasMember("contexts") && document["contexts"].IsArray()) {
        for (const auto& context : document["contexts"].GetArray()) {
            if (context.IsString() && actionMaps_.contains(context.GetString())) {
                contextStack_.push_back(context.GetString());
            }
        }
    }

    if (contextStack_.empty()) {
        if (!actionMaps_.contains(defaultMapName_)) {
            CreateActionMap(defaultMapName_);
        }
        PushContext(defaultMapName_);
    }

    return true;
}

void InputManager::OnKeyPressed(KeyCode key, bool isRepeat) {
    auto& state = keyStates_[key];
    if (!state.isDown) {
        state.isDown      = true;
        state.justPressed = true;
    } else if (isRepeat) {
        // Re-fire justPressed on key-repeat so that consumers like the console
        // can detect held backspace / arrow keys every frame.
        state.justPressed = true;
    }
}

void InputManager::OnKeyReleased(KeyCode key) {
    auto& state = keyStates_[key];
    if (state.isDown) {
        state.isDown       = false;
        state.justReleased = true;
    }
}

void InputManager::OnMouseButtonPressed(MouseButton button) {
    auto& state = mouseButtonStates_[button];
    if (!state.isDown) {
        state.isDown      = true;
        state.justPressed = true;
    }
}

void InputManager::OnMouseButtonReleased(MouseButton button) {
    auto& state = mouseButtonStates_[button];
    if (state.isDown) {
        state.isDown       = false;
        state.justReleased = true;
    }
}

void InputManager::OnMouseMoved(float x, float y) {
    if (firstMouse_) {
        lastMousePosition_ = {x, y};
        firstMouse_        = false;
    }

    mousePosition_ = {x, y};
    mouseDelta_ += mousePosition_ - lastMousePosition_;
    lastMousePosition_ = mousePosition_;
}

void InputManager::OnMouseScrolled(float yOffset) {
    scrollDelta_ += yOffset;
}

void InputManager::OnTextInput(uint32_t codepoint) {
    // Cap the queue so it can't grow unbounded if no consumer is running.
    if (textInputQueue_.size() < 256) {
        textInputQueue_.push_back(codepoint);
    }
}

void InputManager::OnWindowFocusChanged(bool focused) {
    if (!focused) {
        ResetAllInputStates();
        firstMouse_ = true;
    }
}

std::vector<uint32_t> InputManager::ConsumeTextInput() {
    std::vector<uint32_t> out;
    out.swap(textInputQueue_);
    return out;
}

std::string InputManager::KeyToString(KeyCode key) {
    switch (key) {
        case Key::Space:
            return "Space";
        case Key::Tab:
            return "Tab";
        case Key::Enter:
            return "Enter";
        case Key::Escape:
            return "Escape";
        case Key::Backspace:
            return "Backspace";
        case Key::Delete:
            return "Delete";
        case Key::Left:
            return "Left";
        case Key::Right:
            return "Right";
        case Key::Up:
            return "Up";
        case Key::Down:
            return "Down";
        case Key::LeftShift:
            return "LeftShift";
        case Key::RightShift:
            return "RightShift";
        case Key::LeftControl:
            return "LeftControl";
        case Key::RightControl:
            return "RightControl";
        case Key::LeftAlt:
            return "LeftAlt";
        case Key::RightAlt:
            return "RightAlt";
        case Key::GraveAccent:
            return "GraveAccent";
        case Key::MouseX:
            return "MouseX";
        case Key::MouseY:
            return "MouseY";
        case Key::MouseScrollY:
            return "MouseScrollY";
        default:
            break;
    }

    if (key >= Key::A && key <= Key::Z) {
        return std::string(1, static_cast<char>('A' + (key - Key::A)));
    }
    if (key >= Key::D0 && key <= Key::D9) {
        return std::string(1, static_cast<char>('0' + (key - Key::D0)));
    }

    if (key >= Key::F1 && key <= Key::F25) {
        return "F" + std::to_string(static_cast<int>(key - Key::F1) + 1);
    }

    return std::to_string(static_cast<int>(key));
}

std::string InputManager::MouseButtonToString(MouseButton button) {
    switch (button) {
        case Mouse::ButtonLeft:
            return "MouseLeft";
        case Mouse::ButtonRight:
            return "MouseRight";
        case Mouse::ButtonMiddle:
            return "MouseMiddle";
        default:
            return "Mouse" + std::to_string(static_cast<int>(button));
    }
}

std::string InputManager::GamepadButtonToString(GamepadButton button) {
    switch (button) {
        case Gamepad::A:
            return "A";
        case Gamepad::B:
            return "B";
        case Gamepad::X:
            return "X";
        case Gamepad::Y:
            return "Y";
        case Gamepad::LeftBumper:
            return "LeftBumper";
        case Gamepad::RightBumper:
            return "RightBumper";
        case Gamepad::Back:
            return "Back";
        case Gamepad::Start:
            return "Start";
        case Gamepad::Guide:
            return "Guide";
        case Gamepad::LeftThumb:
            return "LeftThumb";
        case Gamepad::RightThumb:
            return "RightThumb";
        case Gamepad::DPadUp:
            return "DPadUp";
        case Gamepad::DPadRight:
            return "DPadRight";
        case Gamepad::DPadDown:
            return "DPadDown";
        case Gamepad::DPadLeft:
            return "DPadLeft";
        default:
            return std::to_string(button);
    }
}

std::string InputManager::GamepadAxisToString(GamepadAxis axis) {
    switch (axis) {
        case Gamepad::LeftX:
            return "LeftX";
        case Gamepad::LeftY:
            return "LeftY";
        case Gamepad::RightX:
            return "RightX";
        case Gamepad::RightY:
            return "RightY";
        case Gamepad::LeftTrigger:
            return "LeftTrigger";
        case Gamepad::RightTrigger:
            return "RightTrigger";
        default:
            return std::to_string(axis);
    }
}

bool InputManager::TryParseKey(const std::string& token, KeyCode* outKey) {
    if (!outKey || token.empty()) return false;

    const std::string lower = ToLower(token);

    if (lower.size() == 1) {
        const char c = lower[0];
        if (c >= 'a' && c <= 'z') {
            *outKey = static_cast<KeyCode>(Key::A + (c - 'a'));
            return true;
        }
        if (c >= '0' && c <= '9') {
            *outKey = static_cast<KeyCode>(Key::D0 + (c - '0'));
            return true;
        }
        if (c == '`') {
            *outKey = Key::GraveAccent;
            return true;
        }
    }

    static const std::unordered_map<std::string, KeyCode> keyMap = {
        {"space", Key::Space},
        {"tab", Key::Tab},
        {"enter", Key::Enter},
        {"escape", Key::Escape},
        {"esc", Key::Escape},
        {"backspace", Key::Backspace},
        {"delete", Key::Delete},
        {"left", Key::Left},
        {"right", Key::Right},
        {"up", Key::Up},
        {"down", Key::Down},
        {"leftshift", Key::LeftShift},
        {"lshift", Key::LeftShift},
        {"rightshift", Key::RightShift},
        {"rshift", Key::RightShift},
        {"leftcontrol", Key::LeftControl},
        {"lctrl", Key::LeftControl},
        {"rightcontrol", Key::RightControl},
        {"rctrl", Key::RightControl},
        {"leftalt", Key::LeftAlt},
        {"lalt", Key::LeftAlt},
        {"rightalt", Key::RightAlt},
        {"ralt", Key::RightAlt},
        {"graveaccent", Key::GraveAccent},
        {"grave", Key::GraveAccent},
        {"tilde", Key::GraveAccent},
        {"mousex", Key::MouseX},
        {"mousey", Key::MouseY},
        {"mousescrolly", Key::MouseScrollY},
    };

    auto it = keyMap.find(lower);
    if (it != keyMap.end()) {
        *outKey = it->second;
        return true;
    }

    if (lower.size() >= 2 && lower[0] == 'f') {
        const int fIndex = std::atoi(lower.substr(1).c_str());
        if (fIndex >= 1 && fIndex <= 25) {
            *outKey = static_cast<KeyCode>(Key::F1 + (fIndex - 1));
            return true;
        }
    }

    if (std::all_of(lower.begin(), lower.end(), [](unsigned char c) { return std::isdigit(c); })) {
        *outKey = static_cast<KeyCode>(std::atoi(lower.c_str()));
        return true;
    }

    return false;
}

bool InputManager::TryParseMouseButton(const std::string& token, MouseButton* outButton) {
    if (!outButton || token.empty()) return false;

    const std::string lower = ToLower(token);
    if (lower == "mouseleft" || lower == "mouse1" || lower == "left") {
        *outButton = Mouse::ButtonLeft;
        return true;
    }
    if (lower == "mouseright" || lower == "mouse2" || lower == "right") {
        *outButton = Mouse::ButtonRight;
        return true;
    }
    if (lower == "mousemiddle" || lower == "mouse3" || lower == "middle") {
        *outButton = Mouse::ButtonMiddle;
        return true;
    }

    if (lower.rfind("mouse", 0) == 0) {
        const int idx = std::atoi(lower.substr(5).c_str());
        if (idx >= 0 && idx <= 7) {
            *outButton = static_cast<MouseButton>(idx);
            return true;
        }
    }

    if (std::all_of(lower.begin(), lower.end(), [](unsigned char c) { return std::isdigit(c); })) {
        const int idx = std::atoi(lower.c_str());
        if (idx >= 0 && idx <= 7) {
            *outButton = static_cast<MouseButton>(idx);
            return true;
        }
    }

    return false;
}

bool InputManager::TryParseGamepadButton(const std::string& token, GamepadButton* outButton) {
    if (!outButton || token.empty()) return false;

    const std::string lower = ToLower(token);
    static const std::unordered_map<std::string, GamepadButton> map = {
        {"a", Gamepad::A},
        {"b", Gamepad::B},
        {"x", Gamepad::X},
        {"y", Gamepad::Y},
        {"leftbumper", Gamepad::LeftBumper},
        {"lb", Gamepad::LeftBumper},
        {"rightbumper", Gamepad::RightBumper},
        {"rb", Gamepad::RightBumper},
        {"back", Gamepad::Back},
        {"start", Gamepad::Start},
        {"guide", Gamepad::Guide},
        {"leftthumb", Gamepad::LeftThumb},
        {"rightthumb", Gamepad::RightThumb},
        {"dpadup", Gamepad::DPadUp},
        {"dpadright", Gamepad::DPadRight},
        {"dpaddown", Gamepad::DPadDown},
        {"dpadleft", Gamepad::DPadLeft},
    };

    auto it = map.find(lower);
    if (it != map.end()) {
        *outButton = it->second;
        return true;
    }

    if (std::all_of(lower.begin(), lower.end(), [](unsigned char c) { return std::isdigit(c); })) {
        const int idx = std::atoi(lower.c_str());
        if (idx >= 0 && idx < Gamepad::ButtonCount) {
            *outButton = idx;
            return true;
        }
    }

    return false;
}

bool InputManager::TryParseGamepadAxis(const std::string& token, GamepadAxis* outAxis) {
    if (!outAxis || token.empty()) return false;

    const std::string lower = ToLower(token);
    static const std::unordered_map<std::string, GamepadAxis> map = {
        {"leftx", Gamepad::LeftX},
        {"lefty", Gamepad::LeftY},
        {"rightx", Gamepad::RightX},
        {"righty", Gamepad::RightY},
        {"lefttrigger", Gamepad::LeftTrigger},
        {"lt", Gamepad::LeftTrigger},
        {"righttrigger", Gamepad::RightTrigger},
        {"rt", Gamepad::RightTrigger},
    };

    auto it = map.find(lower);
    if (it != map.end()) {
        *outAxis = it->second;
        return true;
    }

    if (std::all_of(lower.begin(), lower.end(), [](unsigned char c) { return std::isdigit(c); })) {
        const int idx = std::atoi(lower.c_str());
        if (idx >= 0 && idx < Gamepad::AxisCount) {
            *outAxis = idx;
            return true;
        }
    }

    return false;
}

InputActionDefinition* InputManager::EnsureAction(const std::string& mapName,
                                                  const std::string& actionName,
                                                  InputActionType type) {
    if (mapName.empty() || actionName.empty()) return nullptr;

    if (!actionMaps_.contains(mapName)) {
        CreateActionMap(mapName, true);
    }

    auto& map = actionMaps_[mapName];
    auto  it  = map.actions.find(actionName);
    if (it == map.actions.end()) {
        InputActionDefinition definition;
        definition.type = type;
        auto [insertedIt, inserted] = map.actions.emplace(actionName, std::move(definition));
        return &insertedIt->second;
    }

    if (it->second.type != type) {
        SE_LOG_WARN("Input action '{}' in map '{}' already exists with a different type", actionName,
                    mapName);
        return nullptr;
    }

    return &it->second;
}

const InputActionDefinition* InputManager::FindAction(const std::string& actionName) const {
    for (auto it = contextStack_.rbegin(); it != contextStack_.rend(); ++it) {
        auto mapIt = actionMaps_.find(*it);
        if (mapIt == actionMaps_.end() || !mapIt->second.enabled) continue;

        auto actionIt = mapIt->second.actions.find(actionName);
        if (actionIt != mapIt->second.actions.end()) {
            return &actionIt->second;
        }

        if (mapIt->second.consumeLowerPriority) {
            return nullptr;
        }
    }

    auto defaultMapIt = actionMaps_.find(defaultMapName_);
    if (defaultMapIt != actionMaps_.end()) {
        auto actionIt = defaultMapIt->second.actions.find(actionName);
        if (actionIt != defaultMapIt->second.actions.end()) {
            return &actionIt->second;
        }
    }

    return nullptr;
}

bool InputManager::AddBinding(const std::string& mapName, const std::string& actionName,
                              InputActionType type, const InputBinding& binding) {
    auto* action = EnsureAction(mapName, actionName, type);
    if (!action) return false;

    auto duplicate = std::find_if(action->bindings.begin(), action->bindings.end(),
                                  [&](const InputBinding& existing) {
                                      return BindingEquals(existing, binding);
                                  });
    if (duplicate != action->bindings.end()) return true;

    action->bindings.push_back(binding);
    return true;
}

float InputManager::EvaluateAxisBinding(const InputBinding& binding) const {
    switch (binding.kind) {
        case InputBindingKind::Key:
            return IsKeyDown(binding.key) ? binding.scale : 0.0f;
        case InputBindingKind::MouseButton:
            return IsMouseButtonDown(binding.mouseButton) ? binding.scale : 0.0f;
        case InputBindingKind::MouseAxisX:
            return mouseDelta_.x * binding.scale;
        case InputBindingKind::MouseAxisY:
            return mouseDelta_.y * binding.scale;
        case InputBindingKind::MouseScrollY:
            return scrollDelta_ * binding.scale;
        case InputBindingKind::GamepadButton:
            return IsGamepadButtonDown(binding.gamepadButton) ? binding.scale : 0.0f;
        case InputBindingKind::GamepadAxis: {
            float axisValue = GetGamepadAxis(binding.gamepadAxis);
            if (binding.invert) axisValue = -axisValue;
            return axisValue * binding.scale;
        }
    }

    return 0.0f;
}

bool InputManager::EvaluateButtonBindingDown(const InputBinding& binding) const {
    switch (binding.kind) {
        case InputBindingKind::Key:
            return IsKeyDown(binding.key);
        case InputBindingKind::MouseButton:
            return IsMouseButtonDown(binding.mouseButton);
        case InputBindingKind::GamepadButton:
            return IsGamepadButtonDown(binding.gamepadButton);
        case InputBindingKind::MouseAxisX:
            return std::abs(mouseDelta_.x) > 1e-4f;
        case InputBindingKind::MouseAxisY:
            return std::abs(mouseDelta_.y) > 1e-4f;
        case InputBindingKind::MouseScrollY:
            return std::abs(scrollDelta_) > 1e-4f;
        case InputBindingKind::GamepadAxis:
            return std::abs(GetGamepadAxis(binding.gamepadAxis)) > 0.25f;
    }

    return false;
}

bool InputManager::EvaluateButtonBindingJustPressed(const InputBinding& binding) const {
    switch (binding.kind) {
        case InputBindingKind::Key:
            return IsKeyJustPressed(binding.key);
        case InputBindingKind::MouseButton:
            return IsMouseButtonJustPressed(binding.mouseButton);
        case InputBindingKind::GamepadButton:
            return IsGamepadButtonPressed(binding.gamepadButton);
        default:
            return false;
    }
}

bool InputManager::EvaluateButtonBindingJustReleased(const InputBinding& binding) const {
    switch (binding.kind) {
        case InputBindingKind::Key:
            return IsKeyJustReleased(binding.key);
        case InputBindingKind::MouseButton:
            return IsMouseButtonJustReleased(binding.mouseButton);
        case InputBindingKind::GamepadButton:
            return IsGamepadButtonReleased(binding.gamepadButton);
        default:
            return false;
    }
}

void InputManager::ResetPerFrameTransitions() {
    for (auto& [key, state] : keyStates_) {
        state.justPressed  = false;
        state.justReleased = false;
    }

    for (auto& [button, state] : mouseButtonStates_) {
        state.justPressed  = false;
        state.justReleased = false;
    }
}

void InputManager::ResetAllInputStates() {
    for (auto& [key, state] : keyStates_) {
        state.isDown       = false;
        state.justPressed  = false;
        state.justReleased = false;
    }

    for (auto& [button, state] : mouseButtonStates_) {
        state.isDown       = false;
        state.justPressed  = false;
        state.justReleased = false;
    }

    mouseDelta_  = {0.0f, 0.0f};
    scrollDelta_ = 0.0f;
}

bool InputManager::IsMouseAxisKey(KeyCode key, InputBindingKind* outKind) const {
    switch (key) {
        case Key::MouseX:
            if (outKind) *outKind = InputBindingKind::MouseAxisX;
            return true;
        case Key::MouseY:
            if (outKind) *outKind = InputBindingKind::MouseAxisY;
            return true;
        case Key::MouseScrollY:
            if (outKind) *outKind = InputBindingKind::MouseScrollY;
            return true;
        default:
            return false;
    }
}

}  // namespace se
