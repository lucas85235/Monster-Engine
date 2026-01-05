#include "BlendSpacePanel.h"
#include "../core/EditorContext.h"

#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <filesystem>

BlendSpacePanel::BlendSpacePanel(EditorContext& context) : context_(context) {
}

BlendSpacePanel::~BlendSpacePanel() = default;

void BlendSpacePanel::Clear() {
    samples_.clear();
    selectedSample_ = -1;
    nextSampleId_ = 1;
}

int BlendSpacePanel::AddSample(const std::string& clipPath, float x, float y) {
    BlendSample sample;
    sample.id = nextSampleId_++;
    sample.clipPath = clipPath;
    sample.x = std::clamp(x, minX_, maxX_);
    sample.y = std::clamp(y, minY_, maxY_);
    samples_.push_back(sample);
    SyncToContext();
    return sample.id;
}

void BlendSpacePanel::RemoveSample(int sampleId) {
    samples_.erase(
        std::remove_if(samples_.begin(), samples_.end(),
            [sampleId](const BlendSample& s) { return s.id == sampleId; }),
        samples_.end()
    );
    if (selectedSample_ == sampleId) selectedSample_ = -1;
    SyncToContext();
}

void BlendSpacePanel::SetAxisLabels(const std::string& xLabel, const std::string& yLabel) {
    xAxisLabel_ = xLabel;
    yAxisLabel_ = yLabel;
}

void BlendSpacePanel::SyncToContext() {
    if (nodeId_ < 0) return;
    
    EditorBlendSpaceData data;
    data.parameterX = parameterX_;
    data.parameterY = parameterY_;
    data.minX = minX_;
    data.maxX = maxX_;
    data.minY = minY_;
    data.maxY = maxY_;
    
    for (const auto& sample : samples_) {
        EditorBlendSampleData sd;
        sd.id = sample.id;
        sd.clipPath = sample.clipPath;
        sd.x = sample.x;
        sd.y = sample.y;
        data.samples.push_back(sd);
    }
    
    context_.SetBlendSpaceData(nodeId_, data);
}

void BlendSpacePanel::SyncFromContext() {
    if (nodeId_ < 0) return;
    
    const auto* data = context_.GetBlendSpaceData(nodeId_);
    if (!data) return;
    
    samples_.clear();
    
    parameterX_ = data->parameterX;
    parameterY_ = data->parameterY;
    minX_ = data->minX;
    maxX_ = data->maxX;
    minY_ = data->minY;
    maxY_ = data->maxY;
    
    for (const auto& sd : data->samples) {
        BlendSample sample;
        sample.id = sd.id;
        sample.clipPath = sd.clipPath;
        sample.x = sd.x;
        sample.y = sd.y;
        samples_.push_back(sample);
        
        if (sample.id >= nextSampleId_) {
            nextSampleId_ = sample.id + 1;
        }
    }
}

void BlendSpacePanel::Render() {
    if (!isActive_) return;
    
    ImGui::Begin("Blend Space Editor", &isActive_);
    
    ImGui::Text("Parameter X: %s | Parameter Y: %s", 
        parameterX_.empty() ? xAxisLabel_.c_str() : parameterX_.c_str(),
        parameterY_.empty() ? yAxisLabel_.c_str() : parameterY_.c_str());
    ImGui::Separator();
    
    ImGui::Checkbox("Show Preview", &showPreview_);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    ImGui::DragFloat("X", &previewX_, 0.01f, minX_, maxX_);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    ImGui::DragFloat("Y", &previewY_, 0.01f, minY_, maxY_);
    
    ImGui::Separator();
    
    ImGui::Text("Parameter Bindings:");
    char paramXBuf[64];
    strncpy(paramXBuf, parameterX_.c_str(), sizeof(paramXBuf) - 1);
    paramXBuf[sizeof(paramXBuf) - 1] = '\0';
    ImGui::SetNextItemWidth(150.0f);
    if (ImGui::InputText("Param X", paramXBuf, sizeof(paramXBuf))) {
        parameterX_ = paramXBuf;
        SyncToContext();
    }
    ImGui::SameLine();
    char paramYBuf[64];
    strncpy(paramYBuf, parameterY_.c_str(), sizeof(paramYBuf) - 1);
    paramYBuf[sizeof(paramYBuf) - 1] = '\0';
    ImGui::SetNextItemWidth(150.0f);
    if (ImGui::InputText("Param Y", paramYBuf, sizeof(paramYBuf))) {
        parameterY_ = paramYBuf;
        SyncToContext();
    }
    
    ImGui::Separator();
    
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    
    float propertiesWidth = 200.0f;
    float gridSize = fminf(canvasSize.x - propertiesWidth, canvasSize.y) - 40.0f;
    if (gridSize < 100.0f) gridSize = 100.0f;
    
    ImVec2 gridPos(canvasPos.x + 30.0f, canvasPos.y + 10.0f);
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    drawList->AddRectFilled(
        gridPos, 
        ImVec2(gridPos.x + gridSize, gridPos.y + gridSize),
        IM_COL32(40, 40, 45, 255)
    );
    
    RenderGrid();
    RenderTriangulation();
    RenderSamples();
    if (showPreview_) RenderPreviewMarker();
    
    ImGui::InvisibleButton("blendspace_canvas", ImVec2(gridSize + 40.0f, gridSize + 40.0f));
    
    HandleInput();
    
    if (ImGui::BeginPopupContextWindow("BSContextMenu")) {
        ImVec2 mousePos = ImGui::GetMousePosOnOpeningCurrentPopup();
        float normX = (mousePos.x - gridPos.x) / gridSize;
        float normY = (mousePos.y - gridPos.y) / gridSize;
        float sampleX = minX_ + normX * (maxX_ - minX_);
        float sampleY = maxY_ - normY * (maxY_ - minY_);
        
        if (ImGui::MenuItem("Add Sample")) {
            AddSample("", sampleX, sampleY);
        }
        
        if (selectedSample_ >= 0) {
            if (ImGui::MenuItem("Remove Sample")) {
                RemoveSample(selectedSample_);
            }
        }
        
        ImGui::EndPopup();
    }
    
    ImGui::SameLine();
    
    ImGui::BeginChild("SampleProperties", ImVec2(propertiesWidth - 10.0f, gridSize + 40.0f), true);
    RenderSampleProperties();
    ImGui::EndChild();
    
    ImGui::End();
}

void BlendSpacePanel::RenderSampleProperties() {
    ImGui::Text("Sample Properties");
    ImGui::Separator();
    
    if (selectedSample_ < 0) {
        ImGui::TextDisabled("Select a sample to edit");
        return;
    }
    
    BlendSample* sample = nullptr;
    for (auto& s : samples_) {
        if (s.id == selectedSample_) {
            sample = &s;
            break;
        }
    }
    
    if (!sample) return;
    
    if (!clipsScanned_) {
        clipsScanned_ = true;
        availableClips_.clear();
        availableClips_.push_back("");
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator("assets")) {
                if (!entry.is_regular_file()) continue;
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".fbx" || ext == ".gltf" || ext == ".glb") {
                    availableClips_.push_back(entry.path().string());
                }
            }
        } catch (...) {}
    }
    
    std::string displayPath = sample->clipPath.empty() ? "None" : sample->clipPath;
    size_t lastSlash = displayPath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        displayPath = displayPath.substr(lastSlash + 1);
    }
    
    if (ImGui::BeginCombo("Clip", displayPath.c_str())) {
        for (const auto& clip : availableClips_) {
            std::string label = clip.empty() ? "None" : clip;
            size_t slash = label.find_last_of("/\\");
            if (slash != std::string::npos) {
                label = label.substr(slash + 1);
            }
            
            bool selected = (clip == sample->clipPath);
            if (ImGui::Selectable(label.c_str(), selected)) {
                sample->clipPath = clip;
                SyncToContext();
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    
    if (ImGui::DragFloat("X Pos", &sample->x, 0.01f, minX_, maxX_)) {
        SyncToContext();
    }
    if (ImGui::DragFloat("Y Pos", &sample->y, 0.01f, minY_, maxY_)) {
        SyncToContext();
    }
}

void BlendSpacePanel::RenderGrid() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    
    float propertiesWidth = 200.0f;
    float gridSize = fminf(canvasSize.x - propertiesWidth, canvasSize.y) - 40.0f;
    if (gridSize < 100.0f) gridSize = 100.0f;
    
    ImVec2 gridPos(canvasPos.x + 30.0f, canvasPos.y + 10.0f);
    
    int divisions = 8;
    float step = gridSize / divisions;
    
    for (int i = 0; i <= divisions; ++i) {
        ImU32 color = (i == divisions / 2) ? IM_COL32(100, 100, 100, 255) : IM_COL32(60, 60, 65, 255);
        
        drawList->AddLine(
            ImVec2(gridPos.x + i * step, gridPos.y),
            ImVec2(gridPos.x + i * step, gridPos.y + gridSize),
            color
        );
        drawList->AddLine(
            ImVec2(gridPos.x, gridPos.y + i * step),
            ImVec2(gridPos.x + gridSize, gridPos.y + i * step),
            color
        );
    }
    
    drawList->AddRect(gridPos, ImVec2(gridPos.x + gridSize, gridPos.y + gridSize),
        IM_COL32(100, 100, 110, 255));
    
    char buf[32];
    snprintf(buf, sizeof(buf), "%.1f", minX_);
    drawList->AddText(ImVec2(gridPos.x - 5.0f, gridPos.y + gridSize + 5.0f), 
        IM_COL32(200, 200, 200, 255), buf);
    
    snprintf(buf, sizeof(buf), "%.1f", maxX_);
    drawList->AddText(ImVec2(gridPos.x + gridSize - 15.0f, gridPos.y + gridSize + 5.0f), 
        IM_COL32(200, 200, 200, 255), buf);
    
    snprintf(buf, sizeof(buf), "%.1f", maxY_);
    drawList->AddText(ImVec2(gridPos.x - 25.0f, gridPos.y - 5.0f), 
        IM_COL32(200, 200, 200, 255), buf);
    
    snprintf(buf, sizeof(buf), "%.1f", minY_);
    drawList->AddText(ImVec2(gridPos.x - 25.0f, gridPos.y + gridSize - 10.0f), 
        IM_COL32(200, 200, 200, 255), buf);
}

void BlendSpacePanel::RenderSamples() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    
    float propertiesWidth = 200.0f;
    float gridSize = fminf(canvasSize.x - propertiesWidth, canvasSize.y) - 40.0f;
    if (gridSize < 100.0f) gridSize = 100.0f;
    
    ImVec2 gridPos(canvasPos.x + 30.0f, canvasPos.y + 10.0f);
    
    ImGuiIO& io = ImGui::GetIO();
    
    for (auto& sample : samples_) {
        float normX = (sample.x - minX_) / (maxX_ - minX_);
        float normY = (maxY_ - sample.y) / (maxY_ - minY_);
        
        ImVec2 pos(gridPos.x + normX * gridSize, gridPos.y + normY * gridSize);
        
        float radius = 8.0f;
        ImU32 color = (sample.id == selectedSample_) 
            ? IM_COL32(255, 200, 100, 255) 
            : (sample.clipPath.empty() ? IM_COL32(200, 80, 80, 255) : IM_COL32(100, 150, 255, 255));
        
        drawList->AddCircleFilled(pos, radius, color);
        drawList->AddCircle(pos, radius, IM_COL32(255, 255, 255, 200), 0, 2.0f);
        
        if (!sample.clipPath.empty()) {
            std::string clipName = sample.clipPath;
            size_t slash = clipName.find_last_of("/\\");
            if (slash != std::string::npos) clipName = clipName.substr(slash + 1);
            if (clipName.length() > 12) clipName = clipName.substr(0, 9) + "...";
            
            drawList->AddText(ImVec2(pos.x + 10.0f, pos.y - 8.0f), 
                IM_COL32(255, 255, 255, 255), clipName.c_str());
        }
        
        float dist = sqrtf((io.MousePos.x - pos.x) * (io.MousePos.x - pos.x) +
                          (io.MousePos.y - pos.y) * (io.MousePos.y - pos.y));
        
        if (dist <= radius) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                selectedSample_ = sample.id;
                draggingSample_ = sample.id;
            }
        }
        
        if (draggingSample_ == sample.id && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            float newNormX = (io.MousePos.x - gridPos.x) / gridSize;
            float newNormY = (io.MousePos.y - gridPos.y) / gridSize;
            sample.x = std::clamp(minX_ + newNormX * (maxX_ - minX_), minX_, maxX_);
            sample.y = std::clamp(maxY_ - newNormY * (maxY_ - minY_), minY_, maxY_);
        }
    }
    
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (draggingSample_ >= 0) {
            SyncToContext();
        }
        draggingSample_ = -1;
    }
}

void BlendSpacePanel::RenderTriangulation() {
    if (samples_.size() < 3) return;
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    
    float propertiesWidth = 200.0f;
    float gridSize = fminf(canvasSize.x - propertiesWidth, canvasSize.y) - 40.0f;
    if (gridSize < 100.0f) gridSize = 100.0f;
    
    ImVec2 gridPos(canvasPos.x + 30.0f, canvasPos.y + 10.0f);
    
    for (size_t i = 0; i < samples_.size(); ++i) {
        for (size_t j = i + 1; j < samples_.size(); ++j) {
            float normX1 = (samples_[i].x - minX_) / (maxX_ - minX_);
            float normY1 = (maxY_ - samples_[i].y) / (maxY_ - minY_);
            float normX2 = (samples_[j].x - minX_) / (maxX_ - minX_);
            float normY2 = (maxY_ - samples_[j].y) / (maxY_ - minY_);
            
            ImVec2 p1(gridPos.x + normX1 * gridSize, gridPos.y + normY1 * gridSize);
            ImVec2 p2(gridPos.x + normX2 * gridSize, gridPos.y + normY2 * gridSize);
            
            drawList->AddLine(p1, p2, IM_COL32(80, 100, 120, 100), 1.0f);
        }
    }
}

void BlendSpacePanel::RenderPreviewMarker() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    
    float propertiesWidth = 200.0f;
    float gridSize = fminf(canvasSize.x - propertiesWidth, canvasSize.y) - 40.0f;
    if (gridSize < 100.0f) gridSize = 100.0f;
    
    ImVec2 gridPos(canvasPos.x + 30.0f, canvasPos.y + 10.0f);
    
    float normX = (previewX_ - minX_) / (maxX_ - minX_);
    float normY = (maxY_ - previewY_) / (maxY_ - minY_);
    
    ImVec2 pos(gridPos.x + normX * gridSize, gridPos.y + normY * gridSize);
    
    drawList->AddCircleFilled(pos, 6.0f, IM_COL32(255, 100, 100, 200));
    drawList->AddCircle(pos, 6.0f, IM_COL32(255, 255, 255, 255), 0, 2.0f);
    
    float crossSize = 10.0f;
    drawList->AddLine(
        ImVec2(pos.x - crossSize, pos.y),
        ImVec2(pos.x + crossSize, pos.y),
        IM_COL32(255, 100, 100, 255), 2.0f
    );
    drawList->AddLine(
        ImVec2(pos.x, pos.y - crossSize),
        ImVec2(pos.x, pos.y + crossSize),
        IM_COL32(255, 100, 100, 255), 2.0f
    );
}

void BlendSpacePanel::HandleInput() {
    if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_X)) {
        if (selectedSample_ >= 0) {
            RemoveSample(selectedSample_);
        }
    }
}
