#pragma once
#include <memory>
#include <vector>

#include "../Core/Handles.h"
#include "Scene.h"
#include "API.h"

namespace gns
{
    class SceneManager
    {
        static std::vector<std::unique_ptr<Scene>> ScenesList;
        static Scene* ActiveScene;
    public:
        GNS_API static Scene& GetActiveScene();
        GNS_API static Scene* TryGetActiveScene();
        GNS_API static Scene* GetScene(Handle sceneHandle);
        GNS_API static const std::vector<std::unique_ptr<Scene>>& GetLoadedScenes();
        GNS_API static Scene& CreateScene(const std::string& name);
        GNS_API static bool DestroyScene(Handle sceneHandle);
        GNS_API static bool SetActiveScene(Handle sceneHandle);
        GNS_API static bool IsSceneLoaded(Handle sceneHandle);
        GNS_API static void Clear();
        
    };
}
