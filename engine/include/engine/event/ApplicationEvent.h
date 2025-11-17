#pragma once

#include <sstream>

#include "Event.h"

namespace se {

class WindowResizeEvent : public Event {
   public:
    WindowResizeEvent(unsigned int width, unsigned int height) : width_(width), height_(height) {}

    inline unsigned int GetWidth() const {
        return width_;
    }
    inline unsigned int GetHeight() const {
        return height_;
    }

    std::string ToString() const override {
        std::stringstream ss;
        ss << "WindowResizeEvent: " << width_ << ", " << height_;
        return ss.str();
    }

    EVENT_CLASS_TYPE(WindowResize)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
   private:
    unsigned int width_, height_;
};

class WindowMinimizeEvent : public Event {
   public:
    WindowMinimizeEvent(bool minimized) : m_Minimized(minimized) {}

    bool IsMinimized() const {
        return m_Minimized;
    }

    EVENT_CLASS_TYPE(WindowMinimize)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
   private:
    bool m_Minimized = false;
};

class WindowCloseEvent : public Event {
   public:
    WindowCloseEvent() {}

    EVENT_CLASS_TYPE(WindowClose)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class WindowTitleBarHitTestEvent : public Event {
   public:
    WindowTitleBarHitTestEvent(int x, int y, int& hit) : m_X(x), m_Y(y), m_Hit(hit) {}

    inline int GetX() const {
        return m_X;
    }
    inline int GetY() const {
        return m_Y;
    }
    inline void SetHit(bool hit) {
        m_Hit = (int)hit;
    }

    EVENT_CLASS_TYPE(WindowTitleBarHitTest)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
   private:
    int  m_X;
    int  m_Y;
    int& m_Hit;
};

class AppTickEvent : public Event {
   public:
    AppTickEvent() {}

    EVENT_CLASS_TYPE(AppTick)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class AppUpdateEvent : public Event {
   public:
    AppUpdateEvent() {}

    EVENT_CLASS_TYPE(AppUpdate)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class AppRenderEvent : public Event {
   public:
    AppRenderEvent() {}

    EVENT_CLASS_TYPE(AppRender)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};
}  // namespace se