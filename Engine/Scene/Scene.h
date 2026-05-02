#pragma once
#include <string>

#include <glm/glm.hpp>
#include "../Core/Entity.h"
#include "../Core/Handles.h"

namespace gns
{
    struct SceneData
    {
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 proj = glm::mat4(1.0f);
        glm::mat4 viewproj = glm::mat4(1.0f);
        glm::vec4 ambientColor = {1,1,1,1}; // w for intensity
        glm::vec4 sunlightDirection  = {1,1,1,1}; // w for sun power
        glm::vec4 sunlightColor  = {1,1,1,1}; //w unused
    };

    struct Scene
    {
        Handle handle;
        std::string name;
        Entity root;
    };
}
