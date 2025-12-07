#pragma once

// Unified Events Module
// Combines legacy Event system with new EventBus for compatibility

// Legacy event system (blocking, immediate dispatch)
#include "engine/events/LegacyEvent.h"
#include "engine/events/ApplicationEvent.h"
#include "engine/events/KeyEvent.h"
#include "engine/events/MouseEvent.h"
#include "engine/events/InputEvents.h"

// New event system (subscription-based, deferred dispatch)
#include "engine/events/EventBus.h"
#include "engine/events/EventChannel.h"
