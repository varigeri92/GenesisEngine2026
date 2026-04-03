#pragma once
#include "Scene.h"
#include "API.h"

namespace gns
{
    class SceneManager
    {
        static std::vector<Scene*> ScenesList;
        static Scene* ActiveScene;
    public:
        GNS_API static Scene& GetActiveScene();
        GNS_API static Scene& CreateScene(const std::string& name);
        
    };
}