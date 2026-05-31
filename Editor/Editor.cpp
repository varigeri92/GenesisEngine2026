#define SDL_MAIN_HANDLED
#include <cstdio>

#include "EditorCameraSystem.h"
#include "Genesis.h"
#include "TestSystemExternal.h"
#include "../Engine/Renderer/RenderSystem.h"
#include "GenesisGUI_Backend.h"
#include "TestEditorWindow.h"
#include "EditorProjectContext.h"
#include "EditorGUI/Windows/DockingRoot.h"
#include "EditorGUI/Windows/IconBrowserWindow.h"
#include "EditorGUI/Windows/InspectorWindow.h"
#include "EditorGUI/Windows/ProjectFilesWindow.h"
#include "EditorGUI/Windows/ProfilerWindow.h"
#include "EditorGUI/Windows/SceneHierarchyWindow.h"
#include "EditorGUI/Windows/SceneViewWindow.h"
#include "../Engine/Systems/GuiSystem.h"
#include "EditorGUI/Windows/SystemViewer.h"

int main(int argc, char** argv)
{
    GNS_PROFILE_THREAD("Genesis Main Thread");

    {
        GNS_PROFILE_FUNCTION();

        EditorProjectContext projectContext;

        gns::core::EngineConfig cfg = {};
        cfg.headless = false;
        cfg.InitTetsSystem = true;
        cfg.projectRoot = EditorProjectContext::ProjectRootFromCommandLine(argc, argv);
        cfg.editorResourcesRoot = EditorProjectContext::EditorResourcesRootFromCommandLine(argc, argv);

        {
            gns::core::Engine engine(cfg);
            engine.Initialize([&]() {
                projectContext.Validate();
                gns::core::SystemsManager::RegisterSystem<TestSystemExternal>();
                gns::core::SystemsManager::RegisterSystem<EditorCameraSystem>();
                GuiSystem* gui = gns::core::SystemsManager::GetSystem<GuiSystem>();
                gui->RegisterWindow<DockingRoot>("DockingRoot");
                gui->RegisterWindow<SceneHierarchyWindow>("Scene Hierarchy");
                gui->RegisterWindow<InspectorWindow>("Inspector");
                gui->RegisterWindow<SceneViewWindow>("Scene View");
                gui->RegisterWindow<ProjectFilesWindow>("Project Files", projectContext);
                gui->RegisterWindow<ProfilerWindow>("Profiler");
                gui->RegisterWindow<IconBrowserWindow>("Material Icons");
                gui->RegisterWindow<TestEditorWindow>("testEditorWindow");
                gui->RegisterWindow<SystemViewer>("SystemViewer");
            });
            engine.Run();
            engine.ShutDown();
        }
    }

    std::getchar();
}
