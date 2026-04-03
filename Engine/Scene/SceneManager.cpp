#include "gnspch.h"
#include "SceneManager.h"

std::vector<gns::Scene*> gns::SceneManager::ScenesList = {};
gns::Scene* gns::SceneManager::ActiveScene = nullptr;

gns::Scene& gns::SceneManager::GetActiveScene()
{
    if (ActiveScene != nullptr)
        return *ActiveScene;
    
    LOG_WARNING("No active scene return default empty scene");
    return *ScenesList[0];
}

gns::Scene& gns::SceneManager::CreateScene(const std::string& name)
{
    ScenesList.emplace_back();
    Scene* scene = ScenesList[ScenesList.size() - 1];
    scene->name = name;
    scene->root = Entity::CreateEntity(name);
    
    if (ActiveScene == nullptr)
        ActiveScene = scene;
    
    return *scene;
}
