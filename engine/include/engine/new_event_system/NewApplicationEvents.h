#pragma once

namespace se {
struct NewWindowResizeEvent {
    unsigned int width, height;
};
struct NewWindowMinimizeEvent {
    bool minimized;
};
struct NewWindowCloseEvent {};
}  // namespace se