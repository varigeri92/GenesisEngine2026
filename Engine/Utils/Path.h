#pragma once
#include <filesystem>

namespace gns::path
{
    std::filesystem::path ResourcesDirectory();
    std::filesystem::path InResourcesDirectory(std::string relativePath);
    void SetResourcesDirectory();
}
