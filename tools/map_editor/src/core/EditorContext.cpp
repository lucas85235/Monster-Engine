#include "EditorContext.h"

#include "engine/Log.h"

using namespace se;

namespace mst {

EditorContext::EditorContext() {
    SE_LOG_INFO("EditorContext: Initializing...");
    
    commandSystem_ = se::CreateScope<CommandSystem>(eventBus_);
    sceneManager_ = se::CreateScope<SceneManager>(eventBus_);
    entityManager_ = se::CreateScope<EntityManager>(*sceneManager_, eventBus_);
    document_ = se::CreateScope<MapDocument>(*entityManager_, *sceneManager_, eventBus_);
    
    camera_.FocusOnPoint({0.0f, 0.0f, 0.0f});
    
    SE_LOG_INFO("EditorContext: Initialized successfully");
}

EditorContext::~EditorContext() {
    SE_LOG_INFO("EditorContext: Shutting down");
}

}  // namespace mst
