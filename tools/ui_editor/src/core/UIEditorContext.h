#pragma once

#include "Engine.h"
#include "EventBus.h"
#include "CommandSystem.h"
#include "UIDocument.h"

namespace ued {

class UIEditorContext {
public:
    UIEditorContext();
    ~UIEditorContext();
    
    // Core systems
    EventBus& GetEventBus() { return eventBus_; }
    CommandSystem& GetCommandSystem() { return *commandSystem_; }
    
    // Document
    UIDocument& GetDocument() { return *document_; }
    
    // Widget Tree convenience accessor
    UIWidgetTree& GetWidgetTree() { return document_->GetWidgetTree(); }

private:
    EventBus eventBus_;
    se::Scope<CommandSystem> commandSystem_;
    se::Scope<UIDocument> document_;
};

}  // namespace ued
