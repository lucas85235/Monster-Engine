#pragma once

#include <engine/Layer.h>
#include <memory>

class EditorContext;
class AnimGraphPanel;
class PreviewPanel;
class DetailsPanel;
class ParametersPanel;
class AssetBrowserPanel;
class StateMachinePanel;
class BlendSpacePanel;

class AnimationEditorLayer : public se::Layer {
public:
    AnimationEditorLayer();
    ~AnimationEditorLayer() override;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnImGuiRender() override;

private:
    void SetupDockspace();
    void RenderMenuBar();
    void RenderFileDialogs();
    void NewGraph();
    void OpenGraph();
    void SaveGraph();
    void SaveGraphAs();

    std::unique_ptr<EditorContext> context_;
    std::unique_ptr<AnimGraphPanel> graphPanel_;
    std::unique_ptr<PreviewPanel> previewPanel_;
    std::unique_ptr<DetailsPanel> detailsPanel_;
    std::unique_ptr<ParametersPanel> parametersPanel_;
    std::unique_ptr<AssetBrowserPanel> assetBrowserPanel_;
    std::unique_ptr<StateMachinePanel> stateMachinePanel_;
    std::unique_ptr<BlendSpacePanel> blendSpacePanel_;

    std::string currentFilePath_;
    bool showDemoWindow_ = false;
    bool showOpenDialog_ = false;
    bool showSaveDialog_ = false;
};
