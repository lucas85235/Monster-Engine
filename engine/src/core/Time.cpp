#include "engine/core/Time.h"

namespace se {

void Time::Update(float rawDeltaTime) {
    s_deltaTime = rawDeltaTime;
    s_unscaledTime += rawDeltaTime;
    s_time += rawDeltaTime * s_timeScale;
    ++s_frameCount;
}

} // namespace se
