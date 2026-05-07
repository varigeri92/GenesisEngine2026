#pragma once
#include <string>

#include <glm/glm.hpp>

#include "IObject.h"
#include "Texture.h"

namespace gns
{
    struct Shader;

    struct Material : public Object
    {
        Reference<gns::Shader> shader_ref;
        glm::vec4 albedo_color = glm::vec4(1.0f);
        Reference<gns::Texture> albedo_texture;

        Material();
        explicit Material(std::string name);
        Material(Handle handle, std::string name);
    };
}
