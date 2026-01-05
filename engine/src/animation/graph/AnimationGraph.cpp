#include "engine/animation/graph/AnimationGraph.h"
#include "engine/Log.h"

namespace se {
namespace anim {

AnimationGraph::AnimationGraph(const SkinnedModelData* skeleton)
    : skeleton_(skeleton) {
}

void AnimationGraph::SetRootNode(AnimationNodePtr node) {
    rootNode_ = std::move(node);
}

void AnimationGraph::Update(float deltaTime) {
    deltaTime_ = deltaTime;
    totalTime_ += deltaTime;
}

void AnimationGraph::Evaluate(Pose& outPose) {
    if (!rootNode_ || !skeleton_) {
        return;
    }
    
    AnimationContext ctx;
    ctx.skeleton = skeleton_;
    ctx.graph = this;
    ctx.deltaTime = deltaTime_;
    ctx.totalTime = totalTime_;
    
    rootNode_->Evaluate(ctx, outPose);
}

void AnimationGraph::SetFloat(const std::string& name, float value) {
    parameters_[name] = value;
}

void AnimationGraph::SetBool(const std::string& name, bool value) {
    parameters_[name] = value;
}

void AnimationGraph::SetInt(const std::string& name, int value) {
    parameters_[name] = value;
}

void AnimationGraph::SetTrigger(const std::string& name) {
    parameters_[name] = true;
}

void AnimationGraph::ResetTrigger(const std::string& name) {
    parameters_[name] = false;
}

float AnimationGraph::GetFloat(const std::string& name) const {
    auto it = parameters_.find(name);
    if (it != parameters_.end() && std::holds_alternative<float>(it->second)) {
        return std::get<float>(it->second);
    }
    return 0.0f;
}

bool AnimationGraph::GetBool(const std::string& name) const {
    auto it = parameters_.find(name);
    if (it != parameters_.end() && std::holds_alternative<bool>(it->second)) {
        return std::get<bool>(it->second);
    }
    return false;
}

int AnimationGraph::GetInt(const std::string& name) const {
    auto it = parameters_.find(name);
    if (it != parameters_.end() && std::holds_alternative<int>(it->second)) {
        return std::get<int>(it->second);
    }
    return 0;
}

bool AnimationGraph::HasParameter(const std::string& name) const {
    return parameters_.find(name) != parameters_.end();
}

void AnimationGraph::NotifyStateChanged(const std::string& from, const std::string& to) {
    if (stateChangedCallback_) {
        stateChangedCallback_(from, to);
    }
}

}  // namespace anim
}  // namespace se
