#pragma once
#include <functional>

#include "IEventChannel.h"

namespace se {

template <typename EventT>
class EventChannel : public IEventChannel {
   public:
    using ListenerId = std::size_t;
    using ListenerFn = std::function<void(const EventT&)>;

    ListenerId AddListener(const ListenerFn& listener) {
        const ListenerId id = next_listener_id_++;
        pending_listeners_.emplace_back(id, std::move(listener));
        return id;
    }

    void RemoveListener(ListenerId id) {
        pending_remove_listeners_.push_back(id);
    }

    template <typename... Args>
    void Publish(Args&&... args) {
        next_events_.emplace_back(std::forward<Args>(args)...);
    }

    void Dispatch() override {
        ApplyPendingChanges();

        current_events_.swap(next_events_);
        next_events_.clear();

        for (const auto& eventInstance : current_events_) {
            for (const auto& [id, listener] : current_listeners_) { listener(eventInstance); }
        }

        current_events_.clear();
    }

   private:
    void ApplyPendingChanges() {
        // apply all new listeners that were added in this frame
        for (auto& pending : pending_listeners_) { current_listeners_.emplace_back(pending); }
        pending_listeners_.clear();

        // remove current_listeners_
        if (!pending_remove_listeners_.empty())
        {
            auto shouldRemove = [this](const auto& pair)
            {
                auto it = std::find(pending_remove_listeners_.begin(), pending_remove_listeners_.end(), pair.first);
                return it != pending_remove_listeners_.end();
            };

            current_listeners_.erase(
                std::remove_if(current_listeners_.begin(), current_listeners_.end(), shouldRemove),
                current_listeners_.end()
            );

            pending_remove_listeners_.clear();
        }
    }

    int next_listener_id_ = 0;

    // queues
    std::vector<EventT> current_events_;
    std::vector<EventT> next_events_;

    std::vector<std::pair<ListenerId, ListenerFn>> current_listeners_;
    std::vector<std::pair<ListenerId, ListenerFn>> pending_listeners_;
    std::vector<ListenerId>                        pending_remove_listeners_;
};
}  // namespace se