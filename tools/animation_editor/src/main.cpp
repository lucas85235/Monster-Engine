#include <iostream>
#include <memory>

#include "AnimationEditorLayer.h"
#include "engine/Application.h"
#include "engine/Log.h"

int main(int argc, char** argv) {
    std::cout << "Starting Animation Editor..." << std::endl;

    try {
        se::ApplicationSpecification spec;
        spec.Name           = "Animation_Editor";
        spec.WindowWidth    = 1600;
        spec.WindowHeight   = 900;
        spec.VSync          = true;
        spec.Resizable      = true;
        spec.StartMaximized = false;
        spec.EnableImGui    = true;

        se::Application app(spec);

        SE_LOG_INFO("Animation Editor: Application created, pushing layer");
        app.PushLayer<AnimationEditorLayer>();

        SE_LOG_INFO("Animation Editor: Entering main loop");
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception" << std::endl;
        return 1;
    }

    SE_LOG_INFO("Animation Editor shutting down");
    return 0;
}
