#pragma once

namespace se {

// Base class for components that need lifecycle methods.
// Note: For pure ECS, prefer simple POD structs without inheritance.
// This class allows optional Update/FixedUpdate for components that need it.
class Component {
   public:
    virtual ~Component() = default;

    virtual void Enable() {
        is_enabled = true;
    }
    virtual void Disable() {
        is_enabled = false;
    }

    virtual void Update(float delta_time) {}
    virtual void FixedUpdate(float fixed_delta_time) {}
    virtual void CleanUp() {
        Disable();
    }

    bool IsEnabled() const {
        return is_enabled;
    }

   protected:
    bool is_enabled = false;
};
}  // namespace se
