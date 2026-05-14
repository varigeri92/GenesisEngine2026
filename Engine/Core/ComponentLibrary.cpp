#include "gnspch.h"
#include "ComponentLibrary.h"

#define GNS_REGISTER_FIELD(ComponentType, FieldName) \
    ComponentRegistry::RegisterField(componentMeta, #FieldName, &ComponentType::FieldName)

#define GNS_REGISTER_FIELD_FLAGS(ComponentType, FieldName, Flags) \
    ComponentRegistry::RegisterField(componentMeta, #FieldName, &ComponentType::FieldName, Flags)

namespace gns::reflection
{
    template<>
    void RegisterFields<EntityComponent>(ComponentMeta& componentMeta)
    {
        GNS_REGISTER_FIELD_FLAGS(EntityComponent, entity_handle, GNS_EDITOR_READONLY);
        GNS_REGISTER_FIELD(EntityComponent, name);
    }

    template<>
    void RegisterFields<SceneRootComponent>(ComponentMeta& componentMeta)
    {
        GNS_REGISTER_FIELD_FLAGS(SceneRootComponent, scene_handle, GNS_HIDDEN | GNS_EDITOR_READONLY);
    }

    template<>
    void RegisterFields<SceneMemberComponent>(ComponentMeta& componentMeta)
    {
        GNS_REGISTER_FIELD_FLAGS(SceneMemberComponent, scene_handle, GNS_HIDDEN | GNS_EDITOR_READONLY);
    }

    template<>
    void RegisterFields<HierarchyComponent>(ComponentMeta& componentMeta)
    {
        GNS_REGISTER_FIELD_FLAGS(HierarchyComponent, parent, GNS_HIDDEN | GNS_EDITOR_READONLY);
    }

    template<>
    void RegisterFields<Transform>(ComponentMeta& componentMeta)
    {
        GNS_REGISTER_FIELD(Transform, position);
        GNS_REGISTER_FIELD(Transform, rotation);
        GNS_REGISTER_FIELD(Transform, scale);
    }

    template<>
    void RegisterFields<MeshComponent>(ComponentMeta& componentMeta)
    {
        GNS_REGISTER_FIELD(MeshComponent, mesh);
        GNS_REGISTER_FIELD_FLAGS(MeshComponent, material, GNS_EDITOR_READONLY);
    }

    template<>
    void RegisterFields<AmbientLightComponent>(ComponentMeta& componentMeta)
    {
        GNS_REGISTER_FIELD(AmbientLightComponent, color);
    }

    template<>
    void RegisterFields<DirectionalLightComponent>(ComponentMeta& componentMeta)
    {
        GNS_REGISTER_FIELD(DirectionalLightComponent, direction);
        GNS_REGISTER_FIELD(DirectionalLightComponent, color);
    }

    template<>
    void RegisterFields<PointLightComponent>(ComponentMeta& componentMeta)
    {
        GNS_REGISTER_FIELD(PointLightComponent, color);
        GNS_REGISTER_FIELD(PointLightComponent, intensity);
        GNS_REGISTER_FIELD(PointLightComponent, range);
    }

    template<>
    void RegisterFields<SpotLightComponent>(ComponentMeta& componentMeta)
    {
        GNS_REGISTER_FIELD(SpotLightComponent, direction);
        GNS_REGISTER_FIELD(SpotLightComponent, color);
        GNS_REGISTER_FIELD(SpotLightComponent, intensity);
        GNS_REGISTER_FIELD(SpotLightComponent, range);
        GNS_REGISTER_FIELD(SpotLightComponent, innerAngle);
        GNS_REGISTER_FIELD(SpotLightComponent, outerAngle);
    }
}

#undef GNS_REGISTER_FIELD
#undef GNS_REGISTER_FIELD_FLAGS

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
    reflection::ComponentRegistry::RegisterComponent<PointLightComponent>();
    reflection::ComponentRegistry::RegisterComponent<SpotLightComponent>();
}
