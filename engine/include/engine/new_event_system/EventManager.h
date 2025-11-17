#pragma once
#include "engine/event/Event.h"

namespace se {
struct NewEvent {
    EventType type;
};

class EventManager {
   public:
    EventManager()        = default;
    using EventCallbackFn = std::function<void(const NewEvent&)>;

    static void Subscribe(EventType event_type, EventCallbackFn callback) {
        listeners_[event_type].push_back(std::move(callback));
    }

    static void Publish(const NewEvent& event) {

        // executes instantly
        for (const auto& event_function : listeners_[event.type]) { event_function(event); }
    }

   private:
    inline static std::unordered_map<EventType, std::vector<EventCallbackFn>> listeners_;
};
}  // namespace se