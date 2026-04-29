#pragma once
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

struct Transform
{
    glm::mat4 matrix;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
};

struct MeshComponent
{
    gns::Reference<gns::Mesh> mesh;
    gns::Reference<gns::Material> material;
    gns::Reference<gns::Shader> shader;
};