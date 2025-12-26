#include <iostream>
#include <memory>

#include "MapEditorLayer.h"
#include "engine/core/Application.h"
#include "engine/core/Log.h"

int main(int argc, char** argv) {
    std::cout << "Starting Map Editor..." << std::endl;

    try {
        se::ApplicationSpecification spec;
        spec.Name           = "Monster Engine - Map Editor";
        spec.WindowWidth    = 1600;
        spec.WindowHeight   = 900;
        spec.VSync          = true;
        spec.Resizable      = true;
        spec.StartMaximized = false;
        spec.EnableImGui    = true;

        se::Application app(spec);

        SE_LOG_INFO("Map Editor: Application created, pushing layer");
        app.PushLayer<mst::MapEditorLayer>();

        SE_LOG_INFO("Map Editor: Entering main loop");
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception" << std::endl;
        return 1;
    }

    SE_LOG_INFO("Map Editor shutting down");
    return 0;
}
