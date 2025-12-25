#include "engine/Application.h"
#include "engine/Log.h"
#include "UIEditorLayer.h"

#include <iostream>
#include <exception>

int main(int argc, char** argv) {
    std::cout << "Starting UI Editor..." << std::endl;

    try {
        se::ApplicationSpecification spec;
        spec.Name = "UI Editor";
        spec.WindowWidth = 1600;
        spec.WindowHeight = 900;
        spec.VSync = true;
        spec.Resizable = true;
        spec.StartMaximized = true;
        spec.EnableImGui = true;

        se::Application app(spec);

        SE_LOG_INFO("UI Editor: Application created, pushing layer");
        app.PushLayer<ued::UIEditorLayer>();

        SE_LOG_INFO("UI Editor: Entering main loop");
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception" << std::endl;
        return 1;
    }

    SE_LOG_INFO("UI Editor shutting down");
    return 0;
}

