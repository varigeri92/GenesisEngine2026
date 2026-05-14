#include "gnspch.h"
#include "SceneManager.h"

#include "../Core/ComponentLibrary.h"
#include "../Systems/SystemsManager.h"

std::vector<std::unique_ptr<gns::Scene>> gns::SceneManager::ScenesList = {};
gns::Scene* gns::SceneManager::ActiveScene = nullptr;

namespace
{
    void CreateDefaultSceneEntities(gns::Scene& scene)
    {
        gns::Entity ambient = gns::Entity::CreateEntity("Ambient Light", scene.handle, scene.root.entity_handle);
        ambient.AddComponent<AmbientLightComponent>();

        gns::Entity sun = gns::Entity::CreateEntity("Sun Light", scene.handle, scene.root.entity_handle);
        sun.AddComponent<DirectionalLightComponent>();
    }
}

gns::Scene& gns::SceneManager::GetActiveScene()
{
    if (ActiveScene != nullptr)
    {
        return *ActiveScene;
    }

    if (ScenesList.empty())
    {
        return CreateScene("default_scene");
    }

    ActiveScene = ScenesList[0].get();
    return *ActiveScene;
}

gns::Scene* gns::SceneManager::TryGetActiveScene()
{
    return ActiveScene;
}

gns::Scene* gns::SceneManager::GetScene(Handle sceneHandle)
{
    for (const auto& scene : ScenesList)
    {
        if (scene != nullptr && scene->handle == sceneHandle)
        {
            return scene.get();
        }
    }

    return nullptr;
}

const std::vector<std::unique_ptr<gns::Scene>>& gns::SceneManager::GetLoadedScenes()
{
    return ScenesList;
}

gns::Scene& gns::SceneManager::CreateScene(const std::string& name)
{
    return CreateScene(name, false);
}

gns::Scene& gns::SceneManager::CreateScene(const std::string& name, bool createDefaultEntities)
{
    auto scene = std::make_unique<Scene>();
    scene->handle = Handle::New();
    scene->name = name;

    scene->root = Entity::CreateSceneRoot(name, scene->handle);

    Scene& createdScene = *scene;
    ScenesList.emplace_back(std::move(scene));

    if (ActiveScene == nullptr)
    {
        ActiveScene = &createdScene;
    }

    if (createDefaultEntities)
    {
        CreateDefaultSceneEntities(createdScene);
    }

    return createdScene;
}

bool gns::SceneManager::DestroyScene(Handle sceneHandle)
{
    for (auto it = ScenesList.begin(); it != ScenesList.end(); ++it)
    {
        Scene* scene = it->get();
        if (scene == nullptr || scene->handle != sceneHandle)
        {
            continue;
        }

        Entity::DestroyTree(scene->root.entity_handle, true);

        if (ActiveScene == scene)
        {
            ActiveScene = nullptr;
        }

        ScenesList.erase(it);
        if (ActiveScene == nullptr && !ScenesList.empty())
        {
            ActiveScene = ScenesList[0].get();
        }

        return true;
    }

    return false;
}

bool gns::SceneManager::SetActiveScene(Handle sceneHandle)
{
    Scene* scene = GetScene(sceneHandle);
    if (scene == nullptr)
    {
        return false;
    }

    ActiveScene = scene;
    return true;
}

bool gns::SceneManager::IsSceneLoaded(Handle sceneHandle)
{
    return GetScene(sceneHandle) != nullptr;
}

void gns::SceneManager::Clear()
{
    for (const auto& scene : ScenesList)
    {
        if (scene != nullptr)
        {
            Entity::DestroyTree(scene->root.entity_handle, true);
        }
    }

    ScenesList.clear();
    ActiveScene = nullptr;
}
