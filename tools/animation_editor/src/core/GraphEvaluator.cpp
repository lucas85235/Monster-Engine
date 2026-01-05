#include "GraphEvaluator.h"
#include "EditorContext.h"

#include <engine/Log.h>
#include <engine/animation/AnimationClip.h>
#include <engine/animation/AnimationManager.h>
#include <engine/animation/advanced/Pose.h>
#include <engine/animation/graph/AnimationGraph.h>
#include <engine/animation/graph/ClipNode.h>
#include <engine/animation/graph/BlendNode.h>
#include <engine/animation/graph/BlendSpaceNode.h>
#include <engine/animation/graph/StateMachineNode.h>
#include <engine/resources/ModelData.h>

namespace NodeTypes {
    constexpr int Output = 0;
    constexpr int Clip = 1;
    constexpr int Blend = 2;
    constexpr int BlendSpace1D = 3;
    constexpr int BlendSpace2D = 4;
    constexpr int StateMachine = 5;
    constexpr int LayerBlend = 6;
}

GraphEvaluator::GraphEvaluator(EditorContext& context) : context_(context) {
    graph_ = std::make_unique<se::anim::AnimationGraph>();
}

GraphEvaluator::~GraphEvaluator() = default;

int GraphEvaluator::GetConnectedOutputNodeId(int inputPinId) {
    for (const auto& link : context_.GetLinks()) {
        if (link.endPin == inputPinId) {
            for (const auto& node : context_.GetNodes()) {
                for (int outPin : node.outputPins) {
                    if (outPin == link.startPin) {
                        return node.id;
                    }
                }
            }
        }
    }
    return -1;
}

se::anim::IAnimationNode* GraphEvaluator::BuildNodeFromId(int nodeId) {
    if (nodeId < 0) return nullptr;
    
    auto it = builtNodes_.find(nodeId);
    if (it != builtNodes_.end()) {
        return it->second;
    }
    
    const EditorNode* editorNode = nullptr;
    for (const auto& n : context_.GetNodes()) {
        if (n.id == nodeId) {
            editorNode = &n;
            break;
        }
    }
    
    if (!editorNode) return nullptr;
    
    switch (editorNode->nodeType) {
        case NodeTypes::Clip: {
            const std::string& clipPath = context_.GetNodeClipPath(nodeId);
            if (clipPath.empty()) {
                SE_LOG_WARN("[GraphEvaluator] Clip node {} has no animation assigned", nodeId);
                return nullptr;
            }
            
            auto clip = se::AnimationManager::Load(clipPath);
            if (!clip) {
                SE_LOG_ERROR("[GraphEvaluator] Failed to load clip: {}", clipPath);
                return nullptr;
            }
            
            auto clipNode = std::make_unique<se::anim::ClipNode>(editorNode->name, clip, true);
            clipNode->SetPlaybackSpeed(context_.GetNodePlaybackSpeed(nodeId));
            
            auto* rawPtr = clipNode.get();
            context_.StoreBuiltNode(nodeId, std::move(clipNode));
            builtNodes_[nodeId] = rawPtr;
            return rawPtr;
        }
        
        case NodeTypes::Blend: {
            if (editorNode->inputPins.size() < 2) return nullptr;
            
            int inputANodeId = GetConnectedOutputNodeId(editorNode->inputPins[0]);
            int inputBNodeId = GetConnectedOutputNodeId(editorNode->inputPins[1]);
            
            auto* inputA = BuildNodeFromId(inputANodeId);
            auto* inputB = BuildNodeFromId(inputBNodeId);
            
            auto blendNode = std::make_unique<se::anim::BlendNode>(editorNode->name);
            
            if (inputA) {
                auto nodeA = context_.TakeBuiltNode(inputANodeId);
                if (nodeA) blendNode->SetInputA(std::move(nodeA));
            }
            if (inputB) {
                auto nodeB = context_.TakeBuiltNode(inputBNodeId);
                if (nodeB) blendNode->SetInputB(std::move(nodeB));
            }
            
            blendNode->SetBlendWeight(context_.GetNodeBlendAlpha(nodeId));
            
            const std::string& paramBinding = context_.GetNodeBlendParameter(nodeId);
            if (!paramBinding.empty()) {
                blendNode->SetBlendParameter(paramBinding);
            }
            
            auto* rawPtr = blendNode.get();
            context_.StoreBuiltNode(nodeId, std::move(blendNode));
            builtNodes_[nodeId] = rawPtr;
            return rawPtr;
        }
        
        case NodeTypes::BlendSpace2D: {
            const auto* bsData = context_.GetBlendSpaceData(nodeId);
            if (!bsData) {
                SE_LOG_WARN("[GraphEvaluator] BlendSpace2D node {} has no data", nodeId);
                return nullptr;
            }
            
            auto blendSpace = std::make_unique<se::anim::BlendSpace2D>(editorNode->name);
            
            for (const auto& sample : bsData->samples) {
                if (!sample.clipPath.empty()) {
                    auto clip = se::AnimationManager::Load(sample.clipPath);
                    if (clip) {
                        blendSpace->AddSample(clip, glm::vec2(sample.x, sample.y));
                    }
                }
            }
            
            blendSpace->SetBounds(
                glm::vec2(bsData->minX, bsData->minY),
                glm::vec2(bsData->maxX, bsData->maxY)
            );
            blendSpace->Triangulate();
            
            auto bsNode = std::make_unique<se::anim::BlendSpace2DNode>(editorNode->name, std::move(blendSpace));
            
            if (!bsData->parameterX.empty()) {
                bsNode->SetParameterBindingX(bsData->parameterX);
            }
            if (!bsData->parameterY.empty()) {
                bsNode->SetParameterBindingY(bsData->parameterY);
            }
            
            auto* rawPtr = bsNode.get();
            context_.StoreBuiltNode(nodeId, std::move(bsNode));
            builtNodes_[nodeId] = rawPtr;
            return rawPtr;
        }
        
        case NodeTypes::StateMachine: {
            const auto* smData = context_.GetStateMachineData(nodeId);
            if (!smData) {
                SE_LOG_WARN("[GraphEvaluator] StateMachine node {} has no data", nodeId);
                return nullptr;
            }
            
            auto smNode = std::make_unique<se::anim::StateMachineNode>(editorNode->name);
            
            for (const auto& state : smData->states) {
                if (state.clipPath.empty()) continue;
                
                auto clip = se::AnimationManager::Load(state.clipPath);
                if (!clip) continue;
                
                auto clipNode = std::make_unique<se::anim::ClipNode>(state.name, clip, true);
                smNode->AddState(se::anim::AnimationState(state.name, std::move(clipNode)));
                
                if (state.isDefault) {
                    smNode->SetDefaultState(state.name);
                }
            }
            
            for (const auto& trans : smData->transitions) {
                std::string fromName, toName;
                for (const auto& s : smData->states) {
                    if (s.id == trans.fromState) fromName = s.name;
                    if (s.id == trans.toState) toName = s.name;
                }
                
                if (fromName.empty() || toName.empty()) continue;
                
                se::anim::StateTransition transition;
                transition.fromState = fromName;
                transition.toState = toName;
                transition.duration = trans.duration;
                transition.hasExitTime = trans.hasExitTime;
                transition.exitTime = trans.exitTime;
                
                if (!trans.conditionName.empty()) {
                    std::string condName = trans.conditionName;
                    transition.condition = [condName](const se::anim::AnimationGraph& g) {
                        return g.GetBool(condName);
                    };
                }
                
                smNode->AddTransition(transition);
            }
            
            auto* rawPtr = smNode.get();
            context_.StoreBuiltNode(nodeId, std::move(smNode));
            builtNodes_[nodeId] = rawPtr;
            return rawPtr;
        }
        
        default:
            return nullptr;
    }
}

void GraphEvaluator::BuildFromContext() {
    if (!needsRebuild_) return;
    
    SE_LOG_INFO("[GraphEvaluator] Building animation graph from editor context");
    
    builtNodes_.clear();
    context_.ClearBuiltNodes();
    graph_ = std::make_unique<se::anim::AnimationGraph>();
    
    int outputNodeId = -1;
    int outputInputPin = -1;
    
    for (const auto& node : context_.GetNodes()) {
        if (node.nodeType == NodeTypes::Output) {
            outputNodeId = node.id;
            if (!node.inputPins.empty()) {
                outputInputPin = node.inputPins[0];
            }
            break;
        }
    }
    
    if (outputNodeId < 0 || outputInputPin < 0) {
        SE_LOG_WARN("[GraphEvaluator] No output node found in graph");
        needsRebuild_ = false;
        return;
    }
    
    int rootSourceId = GetConnectedOutputNodeId(outputInputPin);
    if (rootSourceId < 0) {
        SE_LOG_WARN("[GraphEvaluator] Output node has no connected input");
        needsRebuild_ = false;
        return;
    }
    
    BuildNodeFromId(rootSourceId);
    
    auto rootNode = context_.TakeBuiltNode(rootSourceId);
    if (rootNode) {
        graph_->SetRootNode(std::move(rootNode));
        SE_LOG_INFO("[GraphEvaluator] Animation graph built successfully");
    } else {
        SE_LOG_ERROR("[GraphEvaluator] Failed to build root node");
    }
    
    needsRebuild_ = false;
}

void GraphEvaluator::Update(float deltaTime) {
    if (graph_) {
        graph_->Update(deltaTime);
    }
}

void GraphEvaluator::Evaluate(se::anim::Pose& outPose, const se::SkinnedModelData* skeleton) {
    if (!skeleton) return;
    
    if (needsRebuild_) {
        BuildFromContext();
    }
    
    if (!graph_) return;
    
    graph_->SetSkeleton(skeleton);
    
    outPose.Resize(skeleton->Bones.size());
    outPose.SetIdentity();
    
    graph_->Evaluate(outPose);
}

void GraphEvaluator::SetFloat(const std::string& name, float value) {
    if (graph_) {
        graph_->SetFloat(name, value);
    }
}

void GraphEvaluator::SetBool(const std::string& name, bool value) {
    if (graph_) {
        graph_->SetBool(name, value);
    }
}

void GraphEvaluator::SetTrigger(const std::string& name) {
    if (graph_) {
        graph_->SetTrigger(name);
    }
}

float GraphEvaluator::GetFloat(const std::string& name) const {
    if (graph_) {
        return graph_->GetFloat(name);
    }
    return 0.0f;
}

bool GraphEvaluator::GetBool(const std::string& name) const {
    if (graph_) {
        return graph_->GetBool(name);
    }
    return false;
}
