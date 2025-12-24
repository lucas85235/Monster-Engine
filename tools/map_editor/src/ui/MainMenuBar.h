#pragma once

namespace mst {

struct MenuBarActions {
    bool newMap    = false;
    bool openMap   = false;
    bool exportMap = false;
    bool exitApp   = false;

    bool createCube     = false;
    bool createSphere   = false;
    bool createCapsule  = false;
    bool createCylinder = false;
    bool createPlane    = false;

    bool deleteSelected    = false;
    bool duplicateSelected = false;

    bool toggleGrid  = false;
    bool resetCamera = false;

    void Reset() { *this = MenuBarActions{}; }
};

class MainMenuBar {
   public:
    MenuBarActions Render();

   private:
    void RenderFileMenu(MenuBarActions& actions);
    void RenderEditMenu(MenuBarActions& actions);
    void RenderCreateMenu(MenuBarActions& actions);
    void RenderViewMenu(MenuBarActions& actions);
};

}  // namespace mst
