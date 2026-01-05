#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

#include <glm.hpp>

struct ImDrawList;
struct ImVec2;

class EditorContext;
class GraphEvaluator;

namespace se {
    class Model;
    class SkinnedModel;
    class Shader;
    class AnimationClip;
    struct SkinnedModelData;
    
    namespace anim {
        class Pose;
    }
}

class PreviewPanel {
public:
    explicit PreviewPanel(EditorContext& context);
    ~PreviewPanel();

    void Render();
    void LoadModel(const std::string& path);
    void LoadAnimation(const std::string& path);

    GraphEvaluator* GetEvaluator() { return evaluator_.get(); }

private:
    void InitFramebuffer();
    void DestroyFramebuffer();
    void ResizeFramebuffer(int width, int height);
    
    void RenderToFramebuffer();
    void RenderViewport();
    void RenderTimeline();
    void RenderPlaybackControls();
    void RenderParametersPanel();
    void HandleCameraInput();
    
    void RenderGrid(ImDrawList* drawList, ImVec2 imagePos);
    void RenderSkeletonOverlay(ImDrawList* drawList, ImVec2 imagePos);
    void Render3DModel();
    
    void UpdateAnimation();
    void ComputeBoneMatrices();

    EditorContext& context_;
    std::unique_ptr<GraphEvaluator> evaluator_;
    std::unique_ptr<se::anim::Pose> currentPose_;
    std::vector<glm::mat4> boneMatrices_;
    std::vector<glm::mat4> globalBoneTransforms_;
    
    unsigned int framebuffer_ = 0;
    unsigned int colorTexture_ = 0;
    unsigned int depthRenderbuffer_ = 0;
    int viewportWidth_ = 800;
    int viewportHeight_ = 600;
    
    float cameraDistance_ = 5.0f;
    float cameraYaw_ = 45.0f;
    float cameraPitch_ = 30.0f;
    float cameraTargetY_ = 1.0f;
    
    bool showSkeleton_ = true;
    bool showGrid_ = true;
    bool showParameters_ = true;
    bool viewportHovered_ = false;
    
    std::shared_ptr<se::SkinnedModel> currentModel_;
    std::shared_ptr<se::Shader> modelShader_;
    std::string loadedModelPath_;
    
    std::shared_ptr<se::AnimationClip> currentAnimation_;
    std::string loadedAnimationPath_;
    
    std::unordered_map<std::string, float> floatParams_;
    std::unordered_map<std::string, bool> boolParams_;
    
    float lastFrameTime_ = 0.0f;
};
