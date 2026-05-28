#pragma once

#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include "../../Engine/Assets/AssetData.h"

namespace editor::projectfiles
{
    struct ProjectFileEntry
    {
        std::filesystem::path path;
        std::string displayName;
        bool isDirectory = false;
        bool isRegularFile = false;
    };

    bool IsMetaFile(const std::filesystem::path& path);
    gns::assets::AssetType GetAssetTypeFromPath(const std::filesystem::path& path);
    std::string GetDisplayName(const std::filesystem::path& path);
    std::vector<ProjectFileEntry> GetVisibleChildren(
        const std::filesystem::path& directory,
        bool showMetaFiles,
        std::error_code& error);
}
