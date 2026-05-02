#include "gnspch.h"
#include "ComponentLibrary.h"

void gns::RegisterCoreComponentReflection()
{
    reflection::ComponentRegistry::RegisterComponent<EntityComponent>();
    reflection::ComponentRegistry::RegisterComponent<SceneRootComponent>();
    reflection::ComponentRegistry::RegisterComponent<SceneMemberComponent>();
    reflection::ComponentRegistry::RegisterComponent<HierarchyComponent>();
    reflection::ComponentRegistry::RegisterComponent<Transform>();
    reflection::ComponentRegistry::RegisterComponent<MeshComponent>();
    reflection::ComponentRegistry::RegisterComponent<AmbientLightComponent>();
    reflection::ComponentRegistry::RegisterComponent<DirectionalLightComponent>();
}
