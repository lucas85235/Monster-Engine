#pragma once
/**
 * IPanel.h - Interface for editor UI panels.
 *
 * Provides a common interface for all dockable panels,
 * enabling uniform rendering and lifecycle management.
 */

namespace mst {

class EditorContext;

class IPanel {
public:
    virtual ~IPanel() = default;
    
    virtual void Render(EditorContext& ctx) = 0;
    
    virtual const char* GetName() const = 0;
    
    virtual bool IsVisible() const { return visible_; }
    virtual void SetVisible(bool visible) { visible_ = visible; }

protected:
    bool visible_ = true;
};

}  // namespace mst
