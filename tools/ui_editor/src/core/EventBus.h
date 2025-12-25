#pragma once

#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <any>

namespace ued {

class EventBus {
public:
    template<typename EventType>
    using Listener = std::function<void(const EventType&)>;

    template<typename EventType>
    void AddListener(Listener<EventType> listener) {
        auto& listeners = listeners_[std::type_index(typeid(EventType))];
        listeners.push_back([listener](const std::any& event) {
            listener(std::any_cast<const EventType&>(event));
        });
    }

    template<typename EventType>
    void Dispatch(const EventType& event) {
        auto it = listeners_.find(std::type_index(typeid(EventType)));
        if (it != listeners_.end()) {
            for (auto& listener : it->second) {
                listener(event);
            }
        }
    }

private:
    std::unordered_map<std::type_index, std::vector<std::function<void(const std::any&)>>> listeners_;
};

}  // namespace ued
