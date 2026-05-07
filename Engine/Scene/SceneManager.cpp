#include "gnspch.h"
#include "SceneManager.h"

#include "../Core/ComponentLibrary.h"
#include "../Systems/SystemsManager.h"

std::vector<std::unique_ptr<gns::Scene>> gns::SceneManager::ScenesList = {};
gns::Scene* gns::SceneManager::ActiveScene = nullptr;

namespace
{
    void DestroyEntityTree(entt::registry& registry, gns::entityHandle entity)
    {
        if (!registry.valid(entity))
        {
            return;
        }

        if (const auto* hierarchy = registry.try_get<HierarchyComponent>(entity))
        {
            const std::vector<gns::entityHandle> children = hierarchy->children;
            for (gns::entityHandle child : children)
            {
                DestroyEntityTree(registry, child);
            }
        }

        registry.destroy(entity);
    }

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
    return CreateScene(name, true);
}

gns::Scene& gns::SceneManager::CreateScene(const std::string& name, bool createDefaultEntities)
{
    auto scene = std::make_unique<Scene>();
    scene->handle = Handle::New();
    scene->name = name;

    auto& registry = core::SystemsManager::GetRegistry();
    const entt::entity rootEntity = registry.create();
    scene->root = Entity(rootEntity);

    auto& entityComponent = registry.emplace<EntityComponent>(rootEntity);
    entityComponent.entity_handle = rootEntity;
    entityComponent.name = name;

    auto& rootComponent = registry.emplace<SceneRootComponent>(rootEntity);
    rootComponent.scene_handle = scene->handle;

    auto& memberComponent = registry.emplace<SceneMemberComponent>(rootEntity);
    memberComponent.scene_handle = scene->handle;

    registry.emplace<HierarchyComponent>(rootEntity);

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

        auto& registry = core::SystemsManager::GetRegistry();
        DestroyEntityTree(registry, scene->root.entity_handle);

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
    auto& registry = core::SystemsManager::GetRegistry();
    for (const auto& scene : ScenesList)
    {
        if (scene != nullptr)
        {
            DestroyEntityTree(registry, scene->root.entity_handle);
        }
    }

    ScenesList.clear();
    ActiveScene = nullptr;
}
