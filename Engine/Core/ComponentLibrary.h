#pragma once
#include <glm/glm.hpp>

#include "Entity.h"

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

