#include "gnspch.h"
#include "Texture.h"

#include <utility>

gns::Texture::Texture(std::string name)
    : Object(std::move(name))
{
}

gns::Texture::Texture(std::string name, std::string path)
    : Object(std::move(name)),
      assetPath(std::move(path))
{
}

gns::Texture::Texture(Handle handle, std::string name, std::string path)
    : Object(handle, std::move(name)),
      assetPath(std::move(path))
{
}

bool gns::Texture::HasPixels() const
{
    return !pixels.empty();
}

void gns::Texture::SetPixels(
    std::vector<uint8_t> pixelData,
    uint32_t textureWidth,
    uint32_t textureHeight,
    uint32_t textureChannels,
    TextureFormat textureFormat)
{
    pixels = std::move(pixelData);
    width = textureWidth;
    height = textureHeight;
    channels = textureChannels;
    format = textureFormat;
}

void gns::Texture::FreeCPUSide()
{
    pixels.clear();
    pixels.shrink_to_fit();
}
