#pragma once

#include <functional>

class EditorContext;
class StateMachinePanel;
class BlendSpacePanel;

class AnimGraphPanel {
public:
    explicit AnimGraphPanel(EditorContext& context);
    ~AnimGraphPanel();

    void Render();

    void SetStateMachinePanel(StateMachinePanel* panel) { smPanel_ = panel; }
    void SetBlendSpacePanel(BlendSpacePanel* panel) { bsPanel_ = panel; }

private:
    void RenderNodes();
    void RenderLinks();
    void HandleNodeCreation();
    void HandleLinkCreation();
    void HandleDeletion();

    void RenderOutputNode(int nodeId, float posX, float posY);
    void RenderClipNode(int nodeId, const char* name, float posX, float posY);
    void RenderBlendNode(int nodeId, float posX, float posY);
    void RenderBlendSpaceNode(int nodeId, float posX, float posY);
    void RenderStateMachineNode(int nodeId, const char* name, float posX, float posY);

    void OpenStateMachineEditor(int nodeId);
    void OpenBlendSpaceEditor(int nodeId);

    EditorContext& context_;
    StateMachinePanel* smPanel_ = nullptr;
    BlendSpacePanel* bsPanel_ = nullptr;
};
