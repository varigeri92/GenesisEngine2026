#pragma once

#include <filesystem>
#include <string>

#include "../../Engine/Assets/AssetManager.h"

namespace editor::assets
{
    class ModelImportController
    {
    public:
        void Begin(const std::filesystem::path& assetPath);
        void DrawPopup();

    private:
        std::filesystem::path m_pendingModelImportPath = {};
        gns::assets::AssetLoadOptions m_loadOptions = {};
        std::string m_error = {};
        bool m_popupOpen = false;
        bool m_shouldOpenPopup = false;

        void Cancel();
        bool Complete();
    };
}
