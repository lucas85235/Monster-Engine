#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace se {
    class AnimationClip;
    struct SkinnedModelData;
    
    namespace anim {
        class Pose;
        class AnimationGraph;
        class IAnimationNode;
    }
}

class EditorContext;

class GraphEvaluator {
public:
    explicit GraphEvaluator(EditorContext& context);
    ~GraphEvaluator();
    
    void BuildFromContext();
    
    void Update(float deltaTime);
    
    void Evaluate(se::anim::Pose& outPose, const se::SkinnedModelData* skeleton);
    
    void SetFloat(const std::string& name, float value);
    void SetBool(const std::string& name, bool value);
    void SetTrigger(const std::string& name);
    
    float GetFloat(const std::string& name) const;
    bool GetBool(const std::string& name) const;
    
    bool NeedsRebuild() const { return needsRebuild_; }
    void MarkNeedsRebuild() { needsRebuild_ = true; }
    
    se::anim::AnimationGraph* GetGraph() { return graph_.get(); }
    
private:
    se::anim::IAnimationNode* BuildNodeFromId(int nodeId);
    int GetConnectedOutputNodeId(int inputPinId);
    
    EditorContext& context_;
    std::unique_ptr<se::anim::AnimationGraph> graph_;
    
    bool needsRebuild_ = true;
    std::unordered_map<int, se::anim::IAnimationNode*> builtNodes_;
};
