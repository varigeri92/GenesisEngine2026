#pragma once
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Entity.h"
#include "Handles.h"

namespace gns
{
    struct Material;
    struct Mesh;
    struct Shader;
}

struct EntityComponent
{
    gns::entityHandle entity_handle;
    std::string name;
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
    gns::entityHandle parent = entt::null;
    std::vector<gns::entityHandle> children;
};

struct Transform
{
    glm::mat4 matrix = glm::mat4(1.0f);
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
    glm::vec3 scale = {1.0f, 1.0f, 1.0f};
};

struct MeshComponent
{
    gns::Reference<gns::Mesh> mesh;
    gns::Reference<gns::Material> material;
    gns::Reference<gns::Shader> shader;
};

struct AmbientLightComponent
{
    glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
};

struct DirectionalLightComponent
{
    glm::vec4 direction = {1.0f, 1.0f, 1.0f, 1.0f};
    glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
};
