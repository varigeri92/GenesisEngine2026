#pragma once

#include <cstdio>
#include <filesystem>
#include <string>

#include "../Engine/Assets/AssetManager.h"
#include "../Engine/Utils/Path.h"
#include "EditorGUI/ProjectFilesModel.h"

namespace editor::dragdrop
{
    inline constexpr const char* ProjectAssetPayloadType = "GNS_PROJECT_ASSET";
    inline constexpr size_t MaxAssetPathLength = 1024;

    struct ProjectAssetPayload
    {
        gns::assets::AssetType assetType = gns::assets::Generic;
        char path[MaxAssetPathLength] = {};
    };

    inline ProjectAssetPayload MakeProjectAssetPayload(const std::filesystem::path& path)
    {
        ProjectAssetPayload payload = {};
        payload.assetType = editor::projectfiles::GetAssetTypeFromPath(path);

        const std::string normalizedPath = gns::path::Normalize(path).string();
        std::snprintf(payload.path, MaxAssetPathLength, "%s", normalizedPath.c_str());
        return payload;
    }
}
