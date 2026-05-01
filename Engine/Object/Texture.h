#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "IObject.h"

namespace gns
{
    namespace DefaultResourceNames
    {
        inline constexpr const char* WhiteTexture = "__Genesis_Default_Texture_White";
        inline constexpr const char* GreyTexture = "__Genesis_Default_Texture_Grey";
        inline constexpr const char* BlackTexture = "__Genesis_Default_Texture_Black";
        inline constexpr const char* ErrorCheckerboardTexture = "__Genesis_Default_Texture_ErrorCheckerboard";
    }

    enum class TextureFormat
    {
        Unknown,
        R8G8B8A8_UNorm,
        R8G8B8_UNorm
    };

    struct Texture : public Object
    {
        std::string assetPath;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t channels = 0;
        TextureFormat format = TextureFormat::Unknown;
        std::vector<uint8_t> pixels;

        Texture() = default;
        explicit Texture(std::string name);
        Texture(std::string name, std::string path);

        Handle Apply();
        bool HasPixels() const;
        void SetPixels(
            std::vector<uint8_t> pixelData,
            uint32_t textureWidth,
            uint32_t textureHeight,
            uint32_t textureChannels,
            TextureFormat textureFormat);
        void FreeCPUSide();
    };
}
