#pragma once

#include "IPanel.h"

#include <imgui.h>

namespace ued {

class StyleEditorPanel : public IPanel {
public:
    void Render(UIEditorContext& ctx) override;
    const char* GetName() const override { return "Style Editor"; }
};

}  // namespace ued
