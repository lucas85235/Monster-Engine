#include "UIEditorContext.h"

#include "engine/core/Log.h"

namespace ued {

UIEditorContext::UIEditorContext() {
    SE_LOG_INFO("Initializing UIEditorContext");
    
    commandSystem_ = se::CreateScope<CommandSystem>();
    document_ = se::CreateScope<UIDocument>();
}

UIEditorContext::~UIEditorContext() {
    SE_LOG_INFO("Destroying UIEditorContext");
}

}  // namespace ued
