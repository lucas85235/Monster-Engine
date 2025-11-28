#pragma once

namespace se {
class Component {
   public:
    virtual ~Component() = default;

    virtual void Enable() {
        is_enabled = true;
    }
    virtual void Disable() {
        is_enabled = false;
    }

    virtual void Update(float delta_time) = 0;
    virtual void FixedUpdate(float fixed_delta_time) {}
    virtual void CleanUp() {
        Disable();
    }

   protected:
    bool is_enabled = false;
};
}  // namespace se
