#include "StateMachinePanel.h"
#include "../core/EditorContext.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cmath>
#include <filesystem>

StateMachinePanel::StateMachinePanel(EditorContext& context) : context_(context) {
}

StateMachinePanel::~StateMachinePanel() = default;

void StateMachinePanel::Clear() {
    states_.clear();
    transitions_.clear();
    selectedState_ = -1;
    selectedTransition_ = -1;
    nextStateId_ = 1;
    nextTransitionId_ = 1;
}

int StateMachinePanel::AddState(const std::string& name, float x, float y) {
    SMState state;
    state.id = nextStateId_++;
    state.name = name;
    state.posX = x;
    state.posY = y;
    
    if (states_.empty()) {
        state.isDefault = true;
    }
    
    states_.push_back(state);
    SyncToContext();
    return state.id;
}

void StateMachinePanel::RemoveState(int stateId) {
    transitions_.erase(
        std::remove_if(transitions_.begin(), transitions_.end(),
            [stateId](const SMTransition& t) { 
                return t.fromState == stateId || t.toState == stateId; 
            }),
        transitions_.end()
    );
    
    states_.erase(
        std::remove_if(states_.begin(), states_.end(),
            [stateId](const SMState& s) { return s.id == stateId; }),
        states_.end()
    );
    
    if (selectedState_ == stateId) selectedState_ = -1;
    SyncToContext();
}

int StateMachinePanel::AddTransition(int fromState, int toState) {
    for (const auto& t : transitions_) {
        if (t.fromState == fromState && t.toState == toState) {
            return -1;
        }
    }
    
    SMTransition trans;
    trans.id = nextTransitionId_++;
    trans.fromState = fromState;
    trans.toState = toState;
    transitions_.push_back(trans);
    SyncToContext();
    return trans.id;
}

void StateMachinePanel::RemoveTransition(int transitionId) {
    transitions_.erase(
        std::remove_if(transitions_.begin(), transitions_.end(),
            [transitionId](const SMTransition& t) { return t.id == transitionId; }),
        transitions_.end()
    );
    if (selectedTransition_ == transitionId) selectedTransition_ = -1;
    SyncToContext();
}

void StateMachinePanel::SetDefaultState(int stateId) {
    for (auto& state : states_) {
        state.isDefault = (state.id == stateId);
    }
    SyncToContext();
}

void StateMachinePanel::SyncToContext() {
    if (nodeId_ < 0) return;
    
    EditorStateMachineData data;
    
    for (const auto& state : states_) {
        EditorStateData sd;
        sd.id = state.id;
        sd.name = state.name;
        sd.clipPath = state.clipPath;
        sd.posX = state.posX;
        sd.posY = state.posY;
        sd.isDefault = state.isDefault;
        sd.isAnyState = state.isAnyState;
        data.states.push_back(sd);
    }
    
    for (const auto& trans : transitions_) {
        EditorTransitionData td;
        td.id = trans.id;
        td.fromState = trans.fromState;
        td.toState = trans.toState;
        td.conditionName = trans.conditionName;
        td.duration = trans.duration;
        td.hasExitTime = trans.hasExitTime;
        td.exitTime = trans.exitTime;
        data.transitions.push_back(td);
    }
    
    context_.SetStateMachineData(nodeId_, data);
}

void StateMachinePanel::SyncFromContext() {
    if (nodeId_ < 0) return;
    
    const auto* data = context_.GetStateMachineData(nodeId_);
    if (!data) return;
    
    states_.clear();
    transitions_.clear();
    
    for (const auto& sd : data->states) {
        SMState state;
        state.id = sd.id;
        state.name = sd.name;
        state.clipPath = sd.clipPath;
        state.posX = sd.posX;
        state.posY = sd.posY;
        state.isDefault = sd.isDefault;
        state.isAnyState = sd.isAnyState;
        states_.push_back(state);
        
        if (state.id >= nextStateId_) {
            nextStateId_ = state.id + 1;
        }
    }
    
    for (const auto& td : data->transitions) {
        SMTransition trans;
        trans.id = td.id;
        trans.fromState = td.fromState;
        trans.toState = td.toState;
        trans.conditionName = td.conditionName;
        trans.duration = td.duration;
        trans.hasExitTime = td.hasExitTime;
        trans.exitTime = td.exitTime;
        transitions_.push_back(trans);
        
        if (trans.id >= nextTransitionId_) {
            nextTransitionId_ = trans.id + 1;
        }
    }
}

void StateMachinePanel::Render() {
    if (!isActive_) return;
    
    ImGui::Begin("State Machine Editor", &isActive_);
    
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    
    float propertiesWidth = 250.0f;
    canvasSize.x -= propertiesWidth;
    
    if (canvasSize.x < 50.0f) canvasSize.x = 50.0f;
    if (canvasSize.y < 50.0f) canvasSize.y = 50.0f;
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    drawList->AddRectFilled(canvasPos, 
        ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
        IM_COL32(30, 30, 35, 255));
    
    float gridStep = 32.0f * canvasScale_;
    for (float x = fmodf(canvasOffset_.x, gridStep); x < canvasSize.x; x += gridStep) {
        drawList->AddLine(
            ImVec2(canvasPos.x + x, canvasPos.y),
            ImVec2(canvasPos.x + x, canvasPos.y + canvasSize.y),
            IM_COL32(50, 50, 55, 255)
        );
    }
    for (float y = fmodf(canvasOffset_.y, gridStep); y < canvasSize.y; y += gridStep) {
        drawList->AddLine(
            ImVec2(canvasPos.x, canvasPos.y + y),
            ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + y),
            IM_COL32(50, 50, 55, 255)
        );
    }
    
    ImGui::InvisibleButton("canvas", canvasSize, 
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    bool canvasHovered = ImGui::IsItemHovered();
    
    HandleInput();
    RenderTransitions();
    RenderStates();
    RenderContextMenu();
    
    if (creatingTransition_ && transitionStartState_ >= 0) {
        for (const auto& state : states_) {
            if (state.id == transitionStartState_) {
                ImVec2 start = GetStateCenter(state);
                start.x += canvasPos.x + canvasOffset_.x;
                start.y += canvasPos.y + canvasOffset_.y;
                
                ImVec2 mousePos = ImGui::GetMousePos();
                drawList->AddLine(start, mousePos, IM_COL32(255, 200, 100, 200), 2.0f);
                break;
            }
        }
    }
    
    ImGui::SameLine();
    
    ImGui::BeginChild("StateProperties", ImVec2(propertiesWidth - 10.0f, 0), true);
    RenderStateProperties();
    ImGui::EndChild();
    
    ImGui::End();
    
    RenderTransitionEditor();
}

void StateMachinePanel::RenderStateProperties() {
    ImGui::Text("State Properties");
    ImGui::Separator();
    
    if (selectedState_ < 0) {
        ImGui::TextDisabled("Select a state to edit");
        return;
    }
    
    SMState* state = nullptr;
    for (auto& s : states_) {
        if (s.id == selectedState_) {
            state = &s;
            break;
        }
    }
    
    if (!state) return;
    
    char nameBuf[128];
    strncpy(nameBuf, state->name.c_str(), sizeof(nameBuf) - 1);
    nameBuf[sizeof(nameBuf) - 1] = '\0';
    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
        state->name = nameBuf;
        SyncToContext();
    }
    
    ImGui::Separator();
    ImGui::Text("Animation Clip");
    
    if (!clipsScanned_) {
        clipsScanned_ = true;
        availableClips_.clear();
        availableClips_.push_back("");
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator("assets")) {
                if (!entry.is_regular_file()) continue;
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".fbx" || ext == ".gltf" || ext == ".glb") {
                    availableClips_.push_back(entry.path().string());
                }
            }
        } catch (...) {}
    }
    
    std::string displayPath = state->clipPath.empty() ? "None" : state->clipPath;
    size_t lastSlash = displayPath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        displayPath = displayPath.substr(lastSlash + 1);
    }
    
    if (ImGui::BeginCombo("Clip", displayPath.c_str())) {
        for (const auto& clip : availableClips_) {
            std::string label = clip.empty() ? "None" : clip;
            size_t slash = label.find_last_of("/\\");
            if (slash != std::string::npos) {
                label = label.substr(slash + 1);
            }
            
            bool selected = (clip == state->clipPath);
            if (ImGui::Selectable(label.c_str(), selected)) {
                state->clipPath = clip;
                SyncToContext();
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    
    ImGui::Separator();
    
    if (ImGui::Checkbox("Default State", &state->isDefault)) {
        if (state->isDefault) {
            SetDefaultState(state->id);
        }
    }
    
    ImGui::Checkbox("Any State", &state->isAnyState);
}

void StateMachinePanel::RenderStates() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    
    ImGuiIO& io = ImGui::GetIO();
    
    for (auto& state : states_) {
        float x = canvasPos.x + state.posX + canvasOffset_.x;
        float y = canvasPos.y + state.posY + canvasOffset_.y;
        
        ImU32 bgColor = IM_COL32(60, 60, 70, 255);
        ImU32 borderColor = IM_COL32(100, 100, 110, 255);
        ImU32 titleColor = IM_COL32(80, 100, 150, 255);
        
        if (state.isDefault) {
            titleColor = IM_COL32(80, 150, 80, 255);
        }
        if (state.isAnyState) {
            titleColor = IM_COL32(150, 100, 80, 255);
            bgColor = IM_COL32(70, 60, 55, 255);
        }
        if (state.id == selectedState_) {
            borderColor = IM_COL32(255, 200, 100, 255);
        }
        
        if (state.clipPath.empty() && !state.isAnyState) {
            bgColor = IM_COL32(80, 50, 50, 255);
        }
        
        ImVec2 rectMin(x, y);
        ImVec2 rectMax(x + state.width, y + state.height);
        
        drawList->AddRectFilled(rectMin, rectMax, bgColor, 6.0f);
        
        drawList->AddRectFilled(
            rectMin, 
            ImVec2(rectMax.x, rectMin.y + 20.0f),
            titleColor, 6.0f, ImDrawFlags_RoundCornersTop
        );
        
        drawList->AddRect(rectMin, rectMax, borderColor, 6.0f, 0, 2.0f);
        
        ImVec2 textPos(x + 8.0f, y + 3.0f);
        drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), state.name.c_str());
        
        if (state.isDefault) {
            ImVec2 tagPos(x + 8.0f, y + 25.0f);
            drawList->AddText(tagPos, IM_COL32(100, 200, 100, 255), "[Default]");
        } else if (!state.clipPath.empty()) {
            std::string clipName = state.clipPath;
            size_t slash = clipName.find_last_of("/\\");
            if (slash != std::string::npos) clipName = clipName.substr(slash + 1);
            if (clipName.length() > 15) clipName = clipName.substr(0, 12) + "...";
            
            ImVec2 clipPos(x + 8.0f, y + 25.0f);
            drawList->AddText(clipPos, IM_COL32(180, 180, 180, 200), clipName.c_str());
        }
        
        bool hovered = io.MousePos.x >= rectMin.x && io.MousePos.x <= rectMax.x &&
                       io.MousePos.y >= rectMin.y && io.MousePos.y <= rectMax.y;
        
        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            if (creatingTransition_ && transitionStartState_ >= 0 && 
                transitionStartState_ != state.id) {
                AddTransition(transitionStartState_, state.id);
                creatingTransition_ = false;
                transitionStartState_ = -1;
            } else {
                selectedState_ = state.id;
                selectedTransition_ = -1;
                draggingState_ = state.id;
                dragOffset_.x = io.MousePos.x - x;
                dragOffset_.y = io.MousePos.y - y;
            }
        }
        
        if (draggingState_ == state.id && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            state.posX = io.MousePos.x - canvasPos.x - canvasOffset_.x - dragOffset_.x;
            state.posY = io.MousePos.y - canvasPos.y - canvasOffset_.y - dragOffset_.y;
        }
        
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (draggingState_ == state.id) {
                SyncToContext();
            }
            draggingState_ = -1;
        }
    }
}

void StateMachinePanel::RenderTransitions() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    
    for (const auto& trans : transitions_) {
        SMState* fromState = nullptr;
        SMState* toState = nullptr;
        
        for (auto& s : states_) {
            if (s.id == trans.fromState) fromState = &s;
            if (s.id == trans.toState) toState = &s;
        }
        
        if (!fromState || !toState) continue;
        
        ImVec2 from = GetConnectionPoint(*fromState, *toState);
        ImVec2 to = GetConnectionPoint(*toState, *fromState);
        
        from.x += canvasPos.x + canvasOffset_.x;
        from.y += canvasPos.y + canvasOffset_.y;
        to.x += canvasPos.x + canvasOffset_.x;
        to.y += canvasPos.y + canvasOffset_.y;
        
        bool selected = (trans.id == selectedTransition_);
        ImU32 color = selected ? IM_COL32(255, 200, 100, 255) : IM_COL32(200, 200, 200, 200);
        
        DrawArrow(from, to, color, selected);
        
        ImVec2 mid((from.x + to.x) * 0.5f, (from.y + to.y) * 0.5f);
        ImGuiIO& io = ImGui::GetIO();
        float dist = sqrtf((io.MousePos.x - mid.x) * (io.MousePos.x - mid.x) + 
                          (io.MousePos.y - mid.y) * (io.MousePos.y - mid.y));
        
        if (dist < 15.0f && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            selectedTransition_ = trans.id;
            selectedState_ = -1;
        }
    }
}

void StateMachinePanel::DrawArrow(ImVec2 from, ImVec2 to, unsigned int color, bool selected) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    float thickness = selected ? 3.0f : 2.0f;
    drawList->AddLine(from, to, color, thickness);
    
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) return;
    
    dx /= len;
    dy /= len;
    
    float arrowSize = 12.0f;
    ImVec2 arrowBase(to.x - dx * arrowSize, to.y - dy * arrowSize);
    
    float perpX = -dy * arrowSize * 0.5f;
    float perpY = dx * arrowSize * 0.5f;
    
    ImVec2 p1(arrowBase.x + perpX, arrowBase.y + perpY);
    ImVec2 p2(arrowBase.x - perpX, arrowBase.y - perpY);
    
    drawList->AddTriangleFilled(to, p1, p2, color);
}

ImVec2 StateMachinePanel::GetStateCenter(const SMState& state) const {
    return ImVec2(state.posX + state.width * 0.5f, state.posY + state.height * 0.5f);
}

ImVec2 StateMachinePanel::GetConnectionPoint(const SMState& from, const SMState& to) const {
    ImVec2 fromCenter = GetStateCenter(from);
    ImVec2 toCenter = GetStateCenter(to);
    
    float dx = toCenter.x - fromCenter.x;
    float dy = toCenter.y - fromCenter.y;
    
    float halfW = from.width * 0.5f;
    float halfH = from.height * 0.5f;
    
    if (fabsf(dx) < 0.001f && fabsf(dy) < 0.001f) {
        return fromCenter;
    }
    
    float scaleX = fabsf(dx) > 0.001f ? halfW / fabsf(dx) : 1000.0f;
    float scaleY = fabsf(dy) > 0.001f ? halfH / fabsf(dy) : 1000.0f;
    float scale = fminf(scaleX, scaleY);
    
    return ImVec2(fromCenter.x + dx * scale, fromCenter.y + dy * scale);
}

void StateMachinePanel::HandleInput() {
    ImGuiIO& io = ImGui::GetIO();
    
    if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        canvasOffset_.x += io.MouseDelta.x;
        canvasOffset_.y += io.MouseDelta.y;
    }
    
    if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_X)) {
        if (selectedState_ >= 0) {
            RemoveState(selectedState_);
        } else if (selectedTransition_ >= 0) {
            RemoveTransition(selectedTransition_);
        }
    }
    
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        creatingTransition_ = false;
        transitionStartState_ = -1;
    }
}

void StateMachinePanel::RenderContextMenu() {
    if (ImGui::BeginPopupContextWindow("SMContextMenu")) {
        ImVec2 mousePos = ImGui::GetMousePosOnOpeningCurrentPopup();
        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        float x = mousePos.x - canvasPos.x - canvasOffset_.x;
        float y = mousePos.y - canvasPos.y - canvasOffset_.y;
        
        if (ImGui::MenuItem("Add State")) {
            int id = AddState("New State", x, y);
        }
        
        if (ImGui::MenuItem("Add Any State")) {
            int id = AddState("Any State", x, y);
            for (auto& s : states_) {
                if (s.id == id) {
                    s.isAnyState = true;
                    break;
                }
            }
            SyncToContext();
        }
        
        ImGui::Separator();
        
        if (selectedState_ >= 0) {
            if (ImGui::MenuItem("Make Transition")) {
                creatingTransition_ = true;
                transitionStartState_ = selectedState_;
            }
            if (ImGui::MenuItem("Set as Default")) {
                SetDefaultState(selectedState_);
            }
            if (ImGui::MenuItem("Delete State")) {
                RemoveState(selectedState_);
            }
        }
        
        if (selectedTransition_ >= 0) {
            if (ImGui::MenuItem("Delete Transition")) {
                RemoveTransition(selectedTransition_);
            }
        }
        
        ImGui::EndPopup();
    }
}

void StateMachinePanel::RenderTransitionEditor() {
    if (selectedTransition_ < 0) return;
    
    SMTransition* trans = nullptr;
    for (auto& t : transitions_) {
        if (t.id == selectedTransition_) {
            trans = &t;
            break;
        }
    }
    if (!trans) return;
    
    ImGui::Begin("Transition Properties");
    
    std::string fromName = "Unknown";
    std::string toName = "Unknown";
    for (const auto& s : states_) {
        if (s.id == trans->fromState) fromName = s.name;
        if (s.id == trans->toState) toName = s.name;
    }
    
    ImGui::Text("From: %s", fromName.c_str());
    ImGui::Text("To: %s", toName.c_str());
    ImGui::Separator();
    
    char condBuf[128];
    strncpy(condBuf, trans->conditionName.c_str(), sizeof(condBuf) - 1);
    condBuf[sizeof(condBuf) - 1] = '\0';
    if (ImGui::InputText("Condition", condBuf, sizeof(condBuf))) {
        trans->conditionName = condBuf;
        SyncToContext();
    }
    
    if (ImGui::DragFloat("Duration", &trans->duration, 0.01f, 0.0f, 2.0f, "%.2f s")) {
        SyncToContext();
    }
    
    if (ImGui::Checkbox("Has Exit Time", &trans->hasExitTime)) {
        SyncToContext();
    }
    if (trans->hasExitTime) {
        if (ImGui::DragFloat("Exit Time", &trans->exitTime, 0.01f, 0.0f, 1.0f)) {
            SyncToContext();
        }
    }
    
    ImGui::End();
}
