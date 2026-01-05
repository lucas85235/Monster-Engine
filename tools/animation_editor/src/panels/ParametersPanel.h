#pragma once

class EditorContext;

class ParametersPanel {
public:
    explicit ParametersPanel(EditorContext& context);
    ~ParametersPanel();

    void Render();

private:
    EditorContext& context_;

    char newParamName_[64] = "";
    int newParamType_ = 0;
};
