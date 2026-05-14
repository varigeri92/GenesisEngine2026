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
}

struct EntityComponent
{
    gns::entityHandle entity_handle;
    std::string name;

    EntityComponent() : entity_handle(gns::NullEntity), name() {}
};

struct SceneRootComponent
{
    gns::Handle scene_handle;
};

struct SceneMemberComponent
{
    gns::Handle scene_handle;
};

struct HierarchyComponent
{
    gns::entityHandle parent;
    std::vector<gns::entityHandle> children;

    HierarchyComponent() : parent(gns::NullEntity), children() {}
};

struct Transform
{
    glm::mat4 matrix;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    Transform() :
        matrix(glm::mat4(1.0f)),
        position{0.0f, 0.0f, 0.0f},
        rotation{0.0f, 0.0f, 0.0f},
        scale{1.0f, 1.0f, 1.0f}
    {}
};

struct MeshComponent
{
    gns::Reference<gns::Mesh> mesh;
    gns::Reference<gns::Material> material;
};

struct AmbientLightComponent
{
    glm::vec4 color;

    AmbientLightComponent() : color{1.0f, 1.0f, 1.0f, 1.0f} {}
};

struct DirectionalLightComponent
{
    glm::vec4 direction;
    glm::vec4 color;

    DirectionalLightComponent() :
        direction{1.0f, 1.0f, 1.0f, 1.0f},
        color{1.0f, 1.0f, 1.0f, 1.0f}
    {}
};

struct PointLightComponent
{
    glm::vec4 color;
    float intensity;
    float range;

    PointLightComponent() :
        color{1.0f, 1.0f, 1.0f, 1.0f},
        intensity(1.0f),
        range(10.0f)
    {}
};

struct SpotLightComponent
{
    glm::vec4 direction;
    glm::vec4 color;
    float intensity;
    float range;
    float innerAngle;
    float outerAngle;

    SpotLightComponent() :
        direction{0.0f, -1.0f, 0.0f, 0.0f},
        color{1.0f, 1.0f, 1.0f, 1.0f},
        intensity(1.0f),
        range(10.0f),
        innerAngle(15.0f),
        outerAngle(30.0f)
    {}
};

namespace gns
{
    GNS_API void RegisterCoreComponentReflection();
}
