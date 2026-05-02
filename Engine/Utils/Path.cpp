#include "gnspch.h"
#include "Path.h"

using namespace std::filesystem;

path resourcesPath;


std::filesystem::path gns::path::ResourcesDirectory()
{
    return resourcesPath;
}

std::filesystem::path gns::path::InResourcesDirectory(std::string relativePath)
{
    std::filesystem::path path = resourcesPath;
    return path.append(relativePath);
}


void gns::path::SetResourcesDirectory()
{
    // NOTE: This assumes the process current directory is one level below the repository/resource root.
    resourcesPath = std::filesystem::current_path();
    LOG_INFO(resourcesPath.string());
    resourcesPath = resourcesPath.parent_path().append("Resources");
    LOG_INFO(resourcesPath.string());
}
