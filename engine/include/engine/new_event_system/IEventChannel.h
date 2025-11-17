#pragma once

namespace se {
class IEventChannel {
   public:
    virtual ~IEventChannel() = default;
    virtual void Dispatch()  = 0;
};
}  // namespace se