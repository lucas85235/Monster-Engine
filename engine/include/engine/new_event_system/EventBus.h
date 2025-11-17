#pragma once
#include "Engine.h"
#include "engine/new_event_system/EventChannel.h"

namespace se {
class EventBus {
   public:

    EventBus() = default;
    ~EventBus() = default;

    // event bus can't be copied
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    template <typename EventT>
    using ListenerId = EventChannel<EventT>::ListenerId;

    template <typename EventT>
    ListenerId<EventT> AddListener(std::function<void(const EventT&)> listener) {
        auto* channel = GetOrCreateChannel<EventT>();
        return channel->AddListener(std::move(listener));
    }

    template <typename EventT, typename... Args>
    void Invoke(Args&&... args) {
        auto* channel = GetOrCreateChannel<EventT>();
        channel->Publish(std::forward<Args>(args)...);
    }

    void dispatch() {
        for (auto& [typeId, channel] : channels_) { channel->Dispatch(); }
    }

    template <typename EventT>
    void RemoveListener(ListenerId<EventT> id) {
        auto* channel = GetChannel<EventT>();
        if (channel) { channel->RemoveListener(id); }
    }

   private:
    template <typename EventT>
    EventChannel<EventT>* GetChannel() {
        EventTypeId typeId = detail::GetTypeId<EventT>();
        auto        it     = channels_.find(typeId);
        if (it == channels_.end()) { return nullptr; }
        return static_cast<EventChannel<EventT>*>(it->second.get());
    }

    template <typename EventT>
    EventChannel<EventT>* GetOrCreateChannel() {
        EventTypeId typeId = detail::GetTypeId<EventT>();
        auto        it     = channels_.find(typeId);
        if (it == channels_.end()) {
            auto  channel = std::make_unique<EventChannel<EventT>>();
            auto* ptr     = channel.get();
            channels_.emplace(typeId, std::move(channel));
            return ptr;
        }
        return static_cast<EventChannel<EventT>*>(it->second.get());
    }

    std::unordered_map<EventTypeId, std::unique_ptr<IEventChannel>> channels_;
};
}  // namespace se