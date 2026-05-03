#pragma once

#include <cctype>
#include <cstdio>
#include <filesystem>
#include <string>

#include "../Engine/Log/Logger.h"
#include "../Engine/Assets/AssetManager.h"
#include "../Engine/Utils/Path.h"

namespace editor::dragdrop
{
    inline constexpr const char* ProjectAssetPayloadType = "GNS_PROJECT_ASSET";
    inline constexpr size_t MaxAssetPathLength = 1024;

    struct ProjectAssetPayload
    {
        gns::assets::AssetType assetType = gns::assets::Generic;
        char path[MaxAssetPathLength] = {};
    };

    inline std::string LowercaseExtension(const std::filesystem::path& path)
    {
        std::string extension = gns::path::Extension(path);
        for (char& c : extension)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return extension;
    }

    inline gns::assets::AssetType AssetTypeFromPath(const std::filesystem::path& path)
    {
        const std::string extension = LowercaseExtension(path);
        if (extension == "gltf" || extension == "glb" || extension == "obj" ||
            extension == "fbx" || extension == "dae" || extension == "3ds" ||
            extension == "blend" || extension == "ply" || extension == "stl")
        {
            return gns::assets::Mesh;
        }

        if (extension == "png" || extension == "jpg" || extension == "jpeg" ||
            extension == "tga" || extension == "bmp")
        {
            return gns::assets::Texture;
        }

        if (extension == "vert" || extension == "frag" || extension == "comp" ||
            extension == "glsl")
        {
            return gns::assets::Shader;
        }

        return gns::assets::Generic;
    }

    inline ProjectAssetPayload MakeProjectAssetPayload(const std::filesystem::path& path)
    {
        ProjectAssetPayload payload = {};
        payload.assetType = AssetTypeFromPath(path);

        const std::string normalizedPath = gns::path::Normalize(path).string();
        std::snprintf(payload.path, MaxAssetPathLength, "%s", normalizedPath.c_str());
        return payload;
    }
}
