#pragma once
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "ComponentReflection.h"
#include "Entity.h"
#include "Handles.h"

namespace gns
{
    struct Material;
    struct Mesh;
    struct Shader;
}

struct GNS_CMP(EntityComponent)
{
    GNS_FIELD(gns::entityHandle, entity_handle, GNS_EDITOR_READONLY)
    GNS_FIELD(std::string, name)

    EntityComponent() : entity_handle(entt::null), name() {}
};

struct GNS_CMP(SceneRootComponent)
{
    GNS_FIELD(gns::Handle, scene_handle, GNS_HIDDEN | GNS_EDITOR_READONLY)
};

struct GNS_CMP(SceneMemberComponent)
{
    GNS_FIELD(gns::Handle, scene_handle, GNS_HIDDEN | GNS_EDITOR_READONLY)
};

struct GNS_CMP(HierarchyComponent)
{
    GNS_FIELD(gns::entityHandle, parent, GNS_HIDDEN | GNS_EDITOR_READONLY)
    std::vector<gns::entityHandle> children;

    HierarchyComponent() : parent(entt::null), children() {}
};

struct GNS_CMP(Transform)
{
    glm::mat4 matrix;
    GNS_FIELD(glm::vec3, position)
    GNS_FIELD(glm::vec3, rotation)
    GNS_FIELD(glm::vec3, scale)

    Transform() :
        matrix(glm::mat4(1.0f)),
        position{0.0f, 0.0f, 0.0f},
        rotation{0.0f, 0.0f, 0.0f},
        scale{1.0f, 1.0f, 1.0f}
    {}
};

struct GNS_CMP(MeshComponent)
{
    GNS_FIELD(gns::Reference<gns::Mesh>, mesh)
    GNS_FIELD(gns::Reference<gns::Material>, material, GNS_EDITOR_READONLY)
    GNS_FIELD(gns::Reference<gns::Shader>, shader, GNS_HIDDEN)
};

struct GNS_CMP(AmbientLightComponent)
{
    GNS_FIELD(glm::vec4, color)

    AmbientLightComponent() : color{1.0f, 1.0f, 1.0f, 1.0f} {}
};

struct GNS_CMP(DirectionalLightComponent)
{
    GNS_FIELD(glm::vec4, direction)
    GNS_FIELD(glm::vec4, color)

    DirectionalLightComponent() :
        direction{1.0f, 1.0f, 1.0f, 1.0f},
        color{1.0f, 1.0f, 1.0f, 1.0f}
    {}
};

struct GNS_CMP(PointLight)
{
    GNS_FIELD(float, intensity)
    GNS_FIELD(float, range)
    GNS_FIELD(glm::vec4, color)

    PointLight() :
        intensity(1),
        range(5),
        color{1.0f, 1.0f, 1.0f, 1.0f}
    {}
};

struct GNS_CMP(SpotLight)
{
    GNS_FIELD(float, intensity)
    GNS_FIELD(float, range)
    GNS_FIELD(float, angle)
    GNS_FIELD(glm::vec4, color)

    SpotLight() :
        intensity(1.f),
        range(5.f),
        angle(45.f),
        color{1.0f, 1.0f, 1.0f, 1.0f}
    {}
};

namespace gns
{
    GNS_API void RegisterCoreComponentReflection();
}
