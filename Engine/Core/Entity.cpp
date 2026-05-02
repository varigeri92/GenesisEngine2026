#include "gnspch.h"
#include "Entity.h"

#include "ComponentLibrary.h"
#include "../Scene/SceneManager.h"

namespace
{
    void RemoveChild(HierarchyComponent& hierarchy, gns::entityHandle child)
    {
        hierarchy.children.erase(
            std::remove(hierarchy.children.begin(), hierarchy.children.end(), child),
            hierarchy.children.end());
    }
}

bool gns::Entity::IsValid() const
{
    return entity_handle != entt::null && core::SystemsManager::GetRegistry().valid(entity_handle);
}

void gns::Entity::Delete()
{
    auto& registry = core::SystemsManager::GetRegistry();
    if (!IsValid())
    {
        return;
    }

    if (registry.any_of<SceneRootComponent>(entity_handle))
    {
        LOG_WARNING("[Entity]: Scene root entities cannot be deleted through Entity::Delete.");
        return;
    }

    if (auto* hierarchy = registry.try_get<HierarchyComponent>(entity_handle))
    {
        const std::vector<gns::entityHandle> children = hierarchy->children;
        for (gns::entityHandle child : children)
        {
            Entity(child).Delete();
        }

        if (hierarchy->parent != entt::null && registry.valid(hierarchy->parent))
        {
            if (auto* parentHierarchy = registry.try_get<HierarchyComponent>(hierarchy->parent))
            {
                RemoveChild(*parentHierarchy, entity_handle);
            }
        }
    }

    registry.destroy(entity_handle);
}

gns::Entity gns::Entity::CreateEntity(const std::string& entityName)
{
    Scene& scene = SceneManager::GetActiveScene();
    return CreateEntity(entityName, scene.handle, scene.root.entity_handle);
}

gns::Entity gns::Entity::CreateEntity(
    const std::string& entityName,
    Handle sceneHandle,
    entityHandle parent)
{
    Scene* scene = SceneManager::GetScene(sceneHandle);
    if (scene == nullptr)
    {
        LOG_WARNING("[Entity]: Invalid scene handle for entity creation. Falling back to active scene.");
        scene = &SceneManager::GetActiveScene();
        sceneHandle = scene->handle;
    }

    if (parent == entt::null)
    {
        parent = scene->root.entity_handle;
    }

    auto& registry = core::SystemsManager::GetRegistry();
    if (!registry.valid(parent))
    {
        LOG_WARNING("[Entity]: Invalid parent for entity creation. Falling back to scene root.");
        parent = scene->root.entity_handle;
    }

    if (const auto* parentMember = registry.try_get<SceneMemberComponent>(parent);
        parentMember == nullptr || parentMember->scene_handle != sceneHandle)
    {
        LOG_WARNING("[Entity]: Parent scene mismatch. Falling back to scene root.");
        parent = scene->root.entity_handle;
    }

    entt::entity entity = registry.create();
    
    auto& entityComponent = registry.emplace<EntityComponent>(entity);
    entityComponent.entity_handle = entity;
    entityComponent.name = entityName;
    
    registry.emplace<Transform>(entity);

    auto& sceneMember = registry.emplace<SceneMemberComponent>(entity);
    sceneMember.scene_handle = sceneHandle;

    registry.emplace<HierarchyComponent>(entity);

    Entity wrappedEntity(entity);
    wrappedEntity.SetParent(parent);
    return wrappedEntity;
}

gns::Entity gns::Entity::Parent() const
{
    if (!IsValid())
    {
        return {};
    }

    auto& registry = core::SystemsManager::GetRegistry();
    const auto* hierarchy = registry.try_get<HierarchyComponent>(entity_handle);
    if (hierarchy == nullptr)
    {
        return {};
    }

    return Entity(hierarchy->parent);
}

const std::vector<gns::entityHandle>& gns::Entity::Children() const
{
    static const std::vector<gns::entityHandle> EmptyChildren = {};

    if (!IsValid())
    {
        return EmptyChildren;
    }

    auto& registry = core::SystemsManager::GetRegistry();
    const auto* hierarchy = registry.try_get<HierarchyComponent>(entity_handle);
    if (hierarchy == nullptr)
    {
        return EmptyChildren;
    }

    return hierarchy->children;
}

void gns::Entity::SetParent(entityHandle parent)
{
    auto& registry = core::SystemsManager::GetRegistry();
    if (!IsValid())
    {
        return;
    }

    if (registry.any_of<SceneRootComponent>(entity_handle))
    {
        LOG_WARNING("[Entity]: Scene root entities cannot be reparented.");
        return;
    }

    if (parent == entity_handle || parent == entt::null || !registry.valid(parent))
    {
        return;
    }

    auto* hierarchy = registry.try_get<HierarchyComponent>(entity_handle);
    auto* parentHierarchy = registry.try_get<HierarchyComponent>(parent);
    auto* sceneMember = registry.try_get<SceneMemberComponent>(entity_handle);
    auto* parentSceneMember = registry.try_get<SceneMemberComponent>(parent);
    if (hierarchy == nullptr || parentHierarchy == nullptr ||
        sceneMember == nullptr || parentSceneMember == nullptr)
    {
        return;
    }

    if (sceneMember->scene_handle != parentSceneMember->scene_handle)
    {
        LOG_WARNING("[Entity]: Cannot parent an entity under a different scene.");
        return;
    }

    if (hierarchy->parent != entt::null && registry.valid(hierarchy->parent))
    {
        if (auto* previousParentHierarchy = registry.try_get<HierarchyComponent>(hierarchy->parent))
        {
            RemoveChild(*previousParentHierarchy, entity_handle);
        }
    }

    hierarchy->parent = parent;
    if (std::find(parentHierarchy->children.begin(), parentHierarchy->children.end(), entity_handle) ==
        parentHierarchy->children.end())
    {
        parentHierarchy->children.push_back(entity_handle);
    }
}
