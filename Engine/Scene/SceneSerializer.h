#pragma once

#include <filesystem>

#include "../API/API.h"
#include "Scene.h"

namespace gns
{
    class SceneSerializer
    {
    public:
        GNS_API static bool SaveScene(const Scene& scene);
        GNS_API static std::filesystem::path GetSceneSavePath(const Scene& scene);
    };
}
