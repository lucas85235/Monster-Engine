#pragma once

class EditorContext;

class DetailsPanel {
public:
    explicit DetailsPanel(EditorContext& context);
    ~DetailsPanel();

    void Render();

private:
    void RenderNodeDetails();
    void RenderLinkDetails();

    EditorContext& context_;
};
