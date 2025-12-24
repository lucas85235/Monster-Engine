#include "engine/ecs/ComponentSystem.h"

#include <algorithm>

#include "engine/Log.h"
#include "engine/ecs/Component.h"
#include "engine/ecs/Scene.h"

namespace se {

ComponentSystem::ComponentSystem(Scene* scene) : scene_(scene) {
    SE_LOG_INFO("ComponentSystem initialized");
}

ComponentSystem::~ComponentSystem() {
    SE_LOG_INFO("ComponentSystem shutting down with {} active components",
                active_components_.size());

    for (auto* component : active_components_) {
        if (component && component->IsEnabled()) { component->OnDestroy(); }
    }

    active_components_.clear();
    pending_start_.clear();
}

void ComponentSystem::ProcessPendingStarts() {
    SE_LOG_CRITICAL("ProcessPendingStarts: {} components pending", pending_start_.size());
    if (pending_start_.empty()) return;

    std::vector<Component*> to_start = std::move(pending_start_);
    pending_start_.clear();

    for (auto* component : to_start) {
        if (!component) continue;

        if (component->IsEnabled()) {
            component->OnEnable();
            component->Start();
            component->MarkAsStarted();
            SE_LOG_CRITICAL("Component started for entity {}", component->GetEntityID());
        }

        active_components_.push_back(component);
    }
}

void ComponentSystem::Update(float dt) {
    is_updating_ = true;

    for (auto* component : active_components_) {
        if (component && component->IsEnabled() && component->HasStarted()) {
            component->Update(dt);
        }
    }

    is_updating_ = false;

    for (auto* c : pending_register_) { RegisterComponent(c); }
    pending_register_.clear();

    for (auto* c : pending_unregister_) { UnregisterComponent(c); }
    pending_unregister_.clear();
}

void ComponentSystem::FixedUpdate(float dt) {
    fixed_time_accumulator_ += dt;

    while (fixed_time_accumulator_ >= FIXED_TIMESTEP) {
        for (auto* component : active_components_) {
            if (component && component->IsEnabled() && component->HasStarted()) {
                component->FixedUpdate(FIXED_TIMESTEP);
            }
        }
        fixed_time_accumulator_ -= FIXED_TIMESTEP;
    }
}

void ComponentSystem::LateUpdate(float dt) {
    for (auto* component : active_components_) {
        if (component && component->IsEnabled() && component->HasStarted()) {
            component->LateUpdate(dt);
        }
    }
}

void ComponentSystem::RegisterComponent(Component* component) {
    if (!component) return;

    if (is_updating_) {
        pending_register_.push_back(component);
        return;
    }

    auto it_active = std::find(active_components_.begin(), active_components_.end(), component);
    if (it_active != active_components_.end()) return;

    auto it_pending = std::find(pending_start_.begin(), pending_start_.end(), component);
    if (it_pending != pending_start_.end()) return;

    pending_start_.push_back(component);
    SE_LOG_CRITICAL("Component registered for entity {}, pending start", component->GetEntityID());
}

void ComponentSystem::UnregisterComponent(Component* component) {
    if (!component) return;

    if (is_updating_) {
        pending_unregister_.push_back(component);
        return;
    }

    if (component->IsEnabled()) {
        component->OnDisable();
        component->OnDestroy();
    }

    auto it = std::find(active_components_.begin(), active_components_.end(), component);
    if (it != active_components_.end()) { active_components_.erase(it); }

    auto it_pending = std::find(pending_start_.begin(), pending_start_.end(), component);
    if (it_pending != pending_start_.end()) { pending_start_.erase(it_pending); }

    SE_LOG_INFO("Component unregistered for entity {}", component->GetEntityID());
}

}  // namespace se
