#include "engine/Application.h"
#include "layers/AnimationTestLayer.h"

int main() {
    se::ApplicationSpecification spec{};
    spec.Name = "Animation System Test";
    spec.WindowWidth = 1600;
    spec.WindowHeight = 900;
    spec.VSync = true;
    spec.StartMaximized = true;

    se::Application app(spec);
    app.PushLayer<AnimationTest::AnimationTestLayer>();

    app.Run();

    return 0;
}
