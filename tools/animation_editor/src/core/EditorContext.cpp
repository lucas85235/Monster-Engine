#include "EditorContext.h"
#include "GraphSerializer.h"
#include <algorithm>
#include <engine/animation/graph/IAnimationNode.h>

const std::string EditorContext::emptyString_;

EditorContext::EditorContext() {
}

EditorContext::~EditorContext() = default;

void EditorContext::Update(float dt) {
    if (isPlaying_) {
        previewTime_ += dt * playbackSpeed_;
    }
}

void EditorContext::CreateNewGraph() {
    nodes_.clear();
    links_.clear();
    nodeProperties_.clear();
    stateMachines_.clear();
    blendSpaces_.clear();
    builtNodes_.clear();
    
    selectedNodeId_ = -1;
    selectedLinkId_ = -1;
    nextNodeId_ = 1;
    nextPinId_ = 1000;
    nextLinkId_ = 10000;
    previewTime_ = 0.0f;
    isPlaying_ = false;
    graphDirty_ = true;

    int outputId = CreateNode(0, 400.0f, 200.0f);
    nodes_[0].name = "Output Pose";
}

bool EditorContext::LoadFromFile(const std::string& path) {
    return GraphSerializer::LoadFromJson(*this, path);
}

bool EditorContext::SaveToFile(const std::string& path) {
    return GraphSerializer::SaveToJson(*this, path);
}

int EditorContext::CreateNode(int nodeType, float x, float y) {
    EditorNode node;
    node.id = nextNodeId_++;
    node.nodeType = nodeType;
    node.posX = x;
    node.posY = y;

    node.inputPins.push_back(nextPinId_++);
    node.outputPins.push_back(nextPinId_++);

    nodes_.push_back(node);
    graphDirty_ = true;
    return node.id;
}

void EditorContext::DeleteNode(int nodeId) {
    links_.erase(
        std::remove_if(links_.begin(), links_.end(),
            [this, nodeId](const EditorLink& link) {
                for (const auto& node : nodes_) {
                    if (node.id == nodeId) {
                        for (int pin : node.inputPins) {
                            if (link.startPin == pin || link.endPin == pin) return true;
                        }
                        for (int pin : node.outputPins) {
                            if (link.startPin == pin || link.endPin == pin) return true;
                        }
                    }
                }
                return false;
            }),
        links_.end()
    );

    nodes_.erase(
        std::remove_if(nodes_.begin(), nodes_.end(),
            [nodeId](const EditorNode& node) { return node.id == nodeId; }),
        nodes_.end()
    );

    nodeProperties_.erase(nodeId);
    stateMachines_.erase(nodeId);
    blendSpaces_.erase(nodeId);

    if (selectedNodeId_ == nodeId) {
        selectedNodeId_ = -1;
    }
    
    graphDirty_ = true;
}

int EditorContext::CreateLink(int startPin, int endPin) {
    EditorLink link;
    link.id = nextLinkId_++;
    link.startPin = startPin;
    link.endPin = endPin;
    links_.push_back(link);
    graphDirty_ = true;
    return link.id;
}

void EditorContext::DeleteLink(int linkId) {
    links_.erase(
        std::remove_if(links_.begin(), links_.end(),
            [linkId](const EditorLink& link) { return link.id == linkId; }),
        links_.end()
    );

    if (selectedLinkId_ == linkId) {
        selectedLinkId_ = -1;
    }
    
    graphDirty_ = true;
}

void EditorContext::SelectNode(int nodeId) {
    selectedNodeId_ = nodeId;
    selectedLinkId_ = -1;
    if (nodeSelectedCallback_) {
        nodeSelectedCallback_(nodeId);
    }
}

void EditorContext::SelectLink(int linkId) {
    selectedLinkId_ = linkId;
    selectedNodeId_ = -1;
}

void EditorContext::ClearSelection() {
    selectedNodeId_ = -1;
    selectedLinkId_ = -1;
}

void EditorContext::Undo() {
}

void EditorContext::Redo() {
}

bool EditorContext::CanUndo() const {
    return false;
}

bool EditorContext::CanRedo() const {
    return false;
}

void EditorContext::SetNodeClipPath(int nodeId, const std::string& path) {
    nodeProperties_[nodeId].clipPath = path;
    graphDirty_ = true;
}

const std::string& EditorContext::GetNodeClipPath(int nodeId) const {
    auto it = nodeProperties_.find(nodeId);
    if (it != nodeProperties_.end()) {
        return it->second.clipPath;
    }
    return emptyString_;
}

void EditorContext::SetNodePlaybackSpeed(int nodeId, float speed) {
    nodeProperties_[nodeId].playbackSpeed = speed;
}

float EditorContext::GetNodePlaybackSpeed(int nodeId) const {
    auto it = nodeProperties_.find(nodeId);
    if (it != nodeProperties_.end()) {
        return it->second.playbackSpeed;
    }
    return 1.0f;
}

void EditorContext::SetNodeBlendAlpha(int nodeId, float alpha) {
    nodeProperties_[nodeId].blendAlpha = alpha;
}

float EditorContext::GetNodeBlendAlpha(int nodeId) const {
    auto it = nodeProperties_.find(nodeId);
    if (it != nodeProperties_.end()) {
        return it->second.blendAlpha;
    }
    return 0.5f;
}

void EditorContext::SetNodeBlendParameter(int nodeId, const std::string& param) {
    nodeProperties_[nodeId].blendParameter = param;
    graphDirty_ = true;
}

const std::string& EditorContext::GetNodeBlendParameter(int nodeId) const {
    auto it = nodeProperties_.find(nodeId);
    if (it != nodeProperties_.end()) {
        return it->second.blendParameter;
    }
    return emptyString_;
}

void EditorContext::SetStateMachineData(int nodeId, const EditorStateMachineData& data) {
    stateMachines_[nodeId] = data;
    graphDirty_ = true;
}

const EditorStateMachineData* EditorContext::GetStateMachineData(int nodeId) const {
    auto it = stateMachines_.find(nodeId);
    if (it != stateMachines_.end()) {
        return &it->second;
    }
    return nullptr;
}

EditorStateMachineData* EditorContext::GetStateMachineDataMutable(int nodeId) {
    auto it = stateMachines_.find(nodeId);
    if (it != stateMachines_.end()) {
        return &it->second;
    }
    stateMachines_[nodeId] = EditorStateMachineData{};
    return &stateMachines_[nodeId];
}

void EditorContext::SetBlendSpaceData(int nodeId, const EditorBlendSpaceData& data) {
    blendSpaces_[nodeId] = data;
    graphDirty_ = true;
}

const EditorBlendSpaceData* EditorContext::GetBlendSpaceData(int nodeId) const {
    auto it = blendSpaces_.find(nodeId);
    if (it != blendSpaces_.end()) {
        return &it->second;
    }
    return nullptr;
}

EditorBlendSpaceData* EditorContext::GetBlendSpaceDataMutable(int nodeId) {
    auto it = blendSpaces_.find(nodeId);
    if (it != blendSpaces_.end()) {
        return &it->second;
    }
    blendSpaces_[nodeId] = EditorBlendSpaceData{};
    return &blendSpaces_[nodeId];
}

void EditorContext::StoreBuiltNode(int nodeId, se::anim::AnimationNodePtr node) {
    builtNodes_[nodeId] = std::move(node);
}

se::anim::AnimationNodePtr EditorContext::TakeBuiltNode(int nodeId) {
    auto it = builtNodes_.find(nodeId);
    if (it != builtNodes_.end()) {
        auto node = std::move(it->second);
        builtNodes_.erase(it);
        return node;
    }
    return nullptr;
}

void EditorContext::ClearBuiltNodes() {
    builtNodes_.clear();
}
