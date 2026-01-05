#pragma once

#include <string>
#include <vector>

struct ImVec2;
class EditorContext;

struct SMState {
    int id = -1;
    std::string name;
    std::string clipPath;
    float posX = 0.0f;
    float posY = 0.0f;
    float width = 120.0f;
    float height = 60.0f;
    bool isDefault = false;
    bool isAnyState = false;
};

struct SMTransition {
    int id = -1;
    int fromState = -1;
    int toState = -1;
    std::string conditionName;
    float duration = 0.25f;
    bool hasExitTime = false;
    float exitTime = 1.0f;
};

class StateMachinePanel {
public:
    explicit StateMachinePanel(EditorContext& context);
    ~StateMachinePanel();

    void Render();
    void SetActive(bool active) { isActive_ = active; }
    bool IsActive() const { return isActive_; }

    void Clear();
    int AddState(const std::string& name, float x, float y);
    void RemoveState(int stateId);
    int AddTransition(int fromState, int toState);
    void RemoveTransition(int transitionId);
    void SetDefaultState(int stateId);

    void SetNodeId(int nodeId) { nodeId_ = nodeId; }
    int GetNodeId() const { return nodeId_; }

    void SyncToContext();
    void SyncFromContext();

    const std::vector<SMState>& GetStates() const { return states_; }
    const std::vector<SMTransition>& GetTransitions() const { return transitions_; }

private:
    void RenderStates();
    void RenderTransitions();
    void RenderContextMenu();
    void RenderTransitionEditor();
    void RenderStateProperties();
    void HandleInput();

    void DrawArrow(ImVec2 from, ImVec2 to, unsigned int color, bool selected);
    ImVec2 GetStateCenter(const SMState& state) const;
    ImVec2 GetConnectionPoint(const SMState& from, const SMState& to) const;

    EditorContext& context_;
    bool isActive_ = false;
    int nodeId_ = -1;

    std::vector<SMState> states_;
    std::vector<SMTransition> transitions_;

    int nextStateId_ = 1;
    int nextTransitionId_ = 1;

    int selectedState_ = -1;
    int selectedTransition_ = -1;
    int draggingState_ = -1;
    ImVec2 dragOffset_;

    bool creatingTransition_ = false;
    int transitionStartState_ = -1;

    ImVec2 canvasOffset_ = {0, 0};
    float canvasScale_ = 1.0f;

    std::vector<std::string> availableClips_;
    bool clipsScanned_ = false;
};
