#define SDL_MAIN_HANDLED
#include <iostream>
#include "EditorCameraSystem.h"
#include "Genesis.h"
#include "TestSystemExternal.h"
#include "../Engine/Renderer/RenderSystem.h"
#include "GenesisGUI_Backend.h"
#include "TestEditorWindow.h"
#include "EditorGUI/Windows/DockingRoot.h"
#include "EditorGUI/Windows/IconBrowserWindow.h"
#include "../Engine/Systems/GuiSystem.h"

int main()
{
    gns::core::EngineConfig cfg = {};
    cfg.headless = false;
    cfg.InitTetsSystem = true;

    {
        gns::core::Engine engine(cfg);
        engine.Initialize([&]() {
            gns::core::SystemsManager::RegisterSystem<TestSystemExternal>();
            gns::core::SystemsManager::RegisterSystem<EditorCameraSystem>();
            GuiSystem* gui = gns::core::SystemsManager::GetSystem<GuiSystem>();
            gui->RegisterWindow<DockingRoot>("DockingRoot");
            gui->RegisterWindow<IconBrowserWindow>("Material Icons");
            gui->RegisterWindow<TestEditorWindow>("testEditorWindow");
        });
        engine.Run();
        engine.ShutDown();
    }
    std::getchar();
}

