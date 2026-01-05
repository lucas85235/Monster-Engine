#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>
#include <variant>

namespace se {
namespace anim {
    class IAnimationNode;
    using AnimationNodePtr = std::unique_ptr<IAnimationNode>;
}
}

struct EditorNode {
    int id = -1;
    int nodeType = 0;
    std::string name;
    float posX = 0.0f;
    float posY = 0.0f;
    std::vector<int> inputPins;
    std::vector<int> outputPins;
};

struct EditorLink {
    int id = -1;
    int startPin = -1;
    int endPin = -1;
};

struct EditorStateData {
    int id = -1;
    std::string name;
    std::string clipPath;
    float posX = 0.0f;
    float posY = 0.0f;
    bool isDefault = false;
    bool isAnyState = false;
};

struct EditorTransitionData {
    int id = -1;
    int fromState = -1;
    int toState = -1;
    std::string conditionName;
    float duration = 0.25f;
    bool hasExitTime = false;
    float exitTime = 1.0f;
};

struct EditorStateMachineData {
    std::vector<EditorStateData> states;
    std::vector<EditorTransitionData> transitions;
};

struct EditorBlendSampleData {
    int id = -1;
    std::string clipPath;
    float x = 0.0f;
    float y = 0.0f;
};

struct EditorBlendSpaceData {
    std::vector<EditorBlendSampleData> samples;
    std::string parameterX;
    std::string parameterY;
    float minX = -1.0f;
    float maxX = 1.0f;
    float minY = -1.0f;
    float maxY = 1.0f;
};

struct EditorNodeProperties {
    std::string clipPath;
    float playbackSpeed = 1.0f;
    float blendAlpha = 0.5f;
    std::string blendParameter;
};

class EditorContext {
public:
    EditorContext();
    ~EditorContext();

    void Update(float dt);

    void CreateNewGraph();
    bool LoadFromFile(const std::string& path);
    bool SaveToFile(const std::string& path);

    int CreateNode(int nodeType, float x, float y);
    void DeleteNode(int nodeId);
    int CreateLink(int startPin, int endPin);
    void DeleteLink(int linkId);

    void SelectNode(int nodeId);
    void SelectLink(int linkId);
    void ClearSelection();

    int GetSelectedNode() const { return selectedNodeId_; }
    int GetSelectedLink() const { return selectedLinkId_; }

    void Undo();
    void Redo();
    bool CanUndo() const;
    bool CanRedo() const;

    std::vector<EditorNode>& GetNodes() { return nodes_; }
    const std::vector<EditorNode>& GetNodes() const { return nodes_; }

    std::vector<EditorLink>& GetLinks() { return links_; }
    const std::vector<EditorLink>& GetLinks() const { return links_; }

    float GetPreviewTime() const { return previewTime_; }
    void SetPreviewTime(float t) { previewTime_ = t; }

    bool IsPlaying() const { return isPlaying_; }
    void SetPlaying(bool play) { isPlaying_ = play; }

    float GetPlaybackSpeed() const { return playbackSpeed_; }
    void SetPlaybackSpeed(float speed) { playbackSpeed_ = speed; }

    using NodeCallback = std::function<void(int)>;
    void OnNodeSelected(NodeCallback callback) { nodeSelectedCallback_ = callback; }

    void SetSelectedClip(const std::string& path) { selectedClipPath_ = path; }
    const std::string& GetSelectedClip() const { return selectedClipPath_; }
    
    void SetSelectedModel(const std::string& path) { selectedModelPath_ = path; }
    const std::string& GetSelectedModel() const { return selectedModelPath_; }

    void SetNodeClipPath(int nodeId, const std::string& path);
    const std::string& GetNodeClipPath(int nodeId) const;
    
    void SetNodePlaybackSpeed(int nodeId, float speed);
    float GetNodePlaybackSpeed(int nodeId) const;
    
    void SetNodeBlendAlpha(int nodeId, float alpha);
    float GetNodeBlendAlpha(int nodeId) const;
    
    void SetNodeBlendParameter(int nodeId, const std::string& param);
    const std::string& GetNodeBlendParameter(int nodeId) const;

    void SetStateMachineData(int nodeId, const EditorStateMachineData& data);
    const EditorStateMachineData* GetStateMachineData(int nodeId) const;
    EditorStateMachineData* GetStateMachineDataMutable(int nodeId);
    
    void SetBlendSpaceData(int nodeId, const EditorBlendSpaceData& data);
    const EditorBlendSpaceData* GetBlendSpaceData(int nodeId) const;
    EditorBlendSpaceData* GetBlendSpaceDataMutable(int nodeId);

    void StoreBuiltNode(int nodeId, se::anim::AnimationNodePtr node);
    se::anim::AnimationNodePtr TakeBuiltNode(int nodeId);
    void ClearBuiltNodes();

    void MarkGraphDirty() { graphDirty_ = true; }
    bool IsGraphDirty() const { return graphDirty_; }
    void ClearGraphDirty() { graphDirty_ = false; }

private:
    int nextNodeId_ = 1;
    int nextPinId_ = 1000;
    int nextLinkId_ = 10000;

    std::vector<EditorNode> nodes_;
    std::vector<EditorLink> links_;

    int selectedNodeId_ = -1;
    int selectedLinkId_ = -1;

    float previewTime_ = 0.0f;
    bool isPlaying_ = false;
    float playbackSpeed_ = 1.0f;

    std::string selectedClipPath_;
    std::string selectedModelPath_;

    NodeCallback nodeSelectedCallback_;

    std::unordered_map<int, EditorNodeProperties> nodeProperties_;
    std::unordered_map<int, EditorStateMachineData> stateMachines_;
    std::unordered_map<int, EditorBlendSpaceData> blendSpaces_;
    std::unordered_map<int, se::anim::AnimationNodePtr> builtNodes_;

    bool graphDirty_ = true;

    static const std::string emptyString_;
};
