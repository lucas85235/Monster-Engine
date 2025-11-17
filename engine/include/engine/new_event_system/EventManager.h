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
        // adds in the event_queue_ to be dispatched in a late time in the same frame
        event_queue_.push_back(event);

        if (event_queue_.size() == 10) {
            Dispatch();
        }
    }

    static void Dispatch() {
        for (auto event : event_queue_) {
            for (const auto& listener : listeners_[event.type]) {
                listener(event);
            }
        }
        event_queue_.clear();
    }

   private:
    inline static std::vector<NewEvent> event_queue_;
    inline static std::unordered_map<EventType, std::vector<EventCallbackFn>> listeners_;
};
}  // namespace se