#pragma once

#include <string>
#include <vector>

class EditorContext;

struct BlendSample {
    int id = -1;
    std::string clipPath;
    float x = 0.0f;
    float y = 0.0f;
};

class BlendSpacePanel {
public:
    explicit BlendSpacePanel(EditorContext& context);
    ~BlendSpacePanel();

    void Render();
    void SetActive(bool active) { isActive_ = active; }
    bool IsActive() const { return isActive_; }

    void Clear();
    int AddSample(const std::string& clipPath, float x, float y);
    void RemoveSample(int sampleId);
    void SetAxisLabels(const std::string& xLabel, const std::string& yLabel);

    void SetNodeId(int nodeId) { nodeId_ = nodeId; }
    int GetNodeId() const { return nodeId_; }

    void SyncToContext();
    void SyncFromContext();

    void SetParameterX(const std::string& param) { parameterX_ = param; }
    void SetParameterY(const std::string& param) { parameterY_ = param; }
    const std::string& GetParameterX() const { return parameterX_; }
    const std::string& GetParameterY() const { return parameterY_; }

    const std::vector<BlendSample>& GetSamples() const { return samples_; }

private:
    void RenderGrid();
    void RenderSamples();
    void RenderTriangulation();
    void RenderPreviewMarker();
    void RenderSampleProperties();
    void HandleInput();

    EditorContext& context_;
    bool isActive_ = false;
    int nodeId_ = -1;

    std::vector<BlendSample> samples_;
    int nextSampleId_ = 1;
    int selectedSample_ = -1;
    int draggingSample_ = -1;

    std::string xAxisLabel_ = "Speed";
    std::string yAxisLabel_ = "Direction";
    std::string parameterX_;
    std::string parameterY_;

    float minX_ = -1.0f;
    float maxX_ = 1.0f;
    float minY_ = -1.0f;
    float maxY_ = 1.0f;

    float previewX_ = 0.0f;
    float previewY_ = 0.0f;
    bool showPreview_ = true;

    std::vector<std::string> availableClips_;
    bool clipsScanned_ = false;
};
