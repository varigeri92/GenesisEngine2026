#define SDL_MAIN_HANDLED
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <yaml-cpp/yaml.h>

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
#include "EditorGUI/Windows/SceneHierarchyWindow.h"
#include "EditorGUI/Windows/SceneViewWindow.h"
#include "../Engine/Systems/GuiSystem.h"

namespace
{
    void RunYamlCppSmokeTest(const EditorProjectContext& projectContext)
    {
        const std::filesystem::path smokeTestPath =
            projectContext.ProjectRoot() / "yaml-cpp-smoke-test.yaml";

        YAML::Emitter emitter;
        emitter << YAML::BeginMap;
        emitter << YAML::Key << "name" << YAML::Value << "Genesis YAML smoke test";
        emitter << YAML::Key << "projectFile" << YAML::Value
            << gns::path::ToRelative(projectContext.ProjectFilePath(), projectContext.ProjectRoot()).generic_string();
        emitter << YAML::Key << "version" << YAML::Value << 1;
        emitter << YAML::EndMap;

        {
            std::ofstream file(smokeTestPath);
            if (!file)
            {
                LOG_ERROR("[Editor]: Failed to create YAML smoke test file.");
                LOG_ERROR(smokeTestPath.string());
                return;
            }

            file << emitter.c_str();
        }

        try
        {
            const YAML::Node yaml = YAML::LoadFile(smokeTestPath.string());
            const std::string name = yaml["name"].as<std::string>();
            const std::string projectFile = yaml["projectFile"].as<std::string>();
            const int version = yaml["version"].as<int>();

            LOG_INFO("[Editor]: YAML smoke test loaded.");
            LOG_INFO("name: " + name);
            LOG_INFO("projectFile: " + projectFile);
            LOG_INFO("version: " + std::to_string(version));
        }
        catch (const YAML::Exception& exception)
        {
            LOG_ERROR("[Editor]: Failed to read YAML smoke test file.");
            LOG_ERROR(exception.what());
        }
    }
}

int main(int argc, char** argv)
{
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
            gui->RegisterWindow<IconBrowserWindow>("Material Icons");
            gui->RegisterWindow<TestEditorWindow>("testEditorWindow");
        });
        RunYamlCppSmokeTest(projectContext);
        engine.Run();
        engine.ShutDown();
    }
    std::getchar();
}
