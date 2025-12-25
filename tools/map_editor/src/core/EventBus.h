#pragma once
/**
 * EventBus.h - Observer pattern for decoupled component communication.
 *
 * Allows components to publish and subscribe to events without direct dependencies.
 * Uses type-erased handlers to support any event type.
 */

#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

#include "Engine.h"
#include "engine/ecs/Entity.h"

namespace mst {

// Event types
struct EntityCreatedEvent {
    se::Entity entity;
};

struct EntityDeletedEvent {
    uint32_t entityId;
    std::string entityName;
};

struct SelectionChangedEvent {
    std::vector<se::Entity> selected;
};

struct MapLoadedEvent {
    std::string filename;
    size_t entityCount;
};

struct MapSavedEvent {
    std::string filename;
};

struct MapClearedEvent {};

struct GizmoOperationChangedEvent {
    int operation;  // 0=Translate, 1=Rotate, 2=Scale
};

struct GridToggleEvent {
    bool visible;
};

struct ColliderDebugToggleEvent {
    bool visible;
};

struct CameraFocusRequestEvent {
    se::Vector3 targetPosition;
};

class EventBus {
public:
    using HandlerId = uint64_t;

    template<typename TEvent>
    HandlerId Subscribe(std::function<void(const TEvent&)> handler) {
        auto typeIdx = std::type_index(typeid(TEvent));
        HandlerId id = nextHandlerId_++;
        
        auto wrapper = [handler](const void* eventPtr) {
            handler(*static_cast<const TEvent*>(eventPtr));
        };
        
        handlers_[typeIdx].push_back({id, wrapper});
        return id;
    }

    template<typename TEvent>
    void Unsubscribe(HandlerId id) {
        auto typeIdx = std::type_index(typeid(TEvent));
        auto it = handlers_.find(typeIdx);
        if (it != handlers_.end()) {
            auto& vec = it->second;
            vec.erase(
                std::remove_if(vec.begin(), vec.end(),
                    [id](const HandlerEntry& e) { return e.id == id; }),
                vec.end()
            );
        }
    }

    template<typename TEvent>
    void Publish(const TEvent& event) {
        auto typeIdx = std::type_index(typeid(TEvent));
        auto it = handlers_.find(typeIdx);
        if (it != handlers_.end()) {
            for (const auto& entry : it->second) {
                entry.handler(&event);
            }
        }
    }

    void Clear() {
        handlers_.clear();
    }

private:
    struct HandlerEntry {
        HandlerId id;
        std::function<void(const void*)> handler;
    };

    std::unordered_map<std::type_index, std::vector<HandlerEntry>> handlers_;
    HandlerId nextHandlerId_ = 1;
};

}  // namespace mst
