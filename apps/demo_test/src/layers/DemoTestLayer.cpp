#include "AnimationTestLayer.h"

#include "../components/AdvancedCharacterAnimator.h"

#include "engine/Application.h"
#include "engine/Camera.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/ecs/SkinnedModelComponent.h"
#include "engine/ecs/AnimatorComponent.h"
#include "engine/input/InputManager.h"
#include "engine/input/GamepadManager.h"
#include "engine/input/GamepadCodes.h"
#include "engine/renderer/IBLProcessor.h"
#include "engine/animation/SkinnedModelManager.h"
#include "engine/debug/DebugRenderer.h"
#include "engine/resources/MapLoader.h"
#include "engine/ui/native/world/WorldSpaceUIRenderer.h"
#include "engine/ui/native/UISystem.h"

#include <imgui.h>
#include <filesystem>
#include <cmath>

namespace DemoTest {

} // namespace AnimationTest
