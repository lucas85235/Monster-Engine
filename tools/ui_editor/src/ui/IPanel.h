#pragma once

namespace ued {

class UIEditorContext;

class IPanel {
public:
    virtual ~IPanel() = default;
    
    virtual void Render(UIEditorContext& ctx) = 0;
    virtual const char* GetName() const = 0;
    
    virtual bool IsVisible() const { return visible_; }
    virtual void SetVisible(bool visible) { visible_ = visible; }

protected:
    bool visible_ = true;
};

}  // namespace ued
