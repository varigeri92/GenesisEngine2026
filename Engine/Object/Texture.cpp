#include "gnspch.h"
#include "Texture.h"

#include "../Renderer/RenderSystem.h"
#include "../Systems/SystemsManager.h"

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

gns::Handle gns::Texture::Apply()
{
    gns::RenderSystem* renderSystem = core::SystemsManager::GetSystem<RenderSystem>();
    return renderSystem->ApplyTexture(*this);
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
