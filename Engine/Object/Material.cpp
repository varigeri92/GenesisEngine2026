#include "gnspch.h"
#include "Material.h"

#include <utility>

gns::Material::Material()
    : Object(Handle::New(), "Material"),
      albedo_texture(Handle::CreateFromString(DefaultResourceNames::WhiteTexture))
{
}

gns::Material::Material(std::string name)
    : Object(std::move(name)),
      albedo_texture(Handle::CreateFromString(DefaultResourceNames::WhiteTexture))
{
}
