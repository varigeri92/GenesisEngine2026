#include "gnspch.h"
#include "Entity.h"

#include "ComponentLibrary.h"

gns::Entity gns::Entity::CreateEntity(const std::string& entityName)
{
    auto& registry = core::SystemsManager::GetRegistry();
    entt::entity entity = registry.create();
    
    auto& entityComponent = registry.emplace<EntityComponent>(entity);
    entityComponent.entity_handle   = entity;
    entityComponent.name            = entityName;
    
    auto& transform = registry.emplace<Transform>(entity);
    transform.position  = {0,0,0};
    transform.rotation  = {0,0,0};
    transform.scale     = {1,1,1};
    
    return {entity};
}
