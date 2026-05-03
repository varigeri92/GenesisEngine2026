#pragma once

#include <filesystem>

#include "Genesis.h"

class EditorSelection
{
public:
    enum class Type
    {
        None,
        Entity,
        File
    };

    static void SelectEntity(gns::entityHandle entity);
    static gns::entityHandle GetSelectedEntity();
    static bool IsSelected(gns::entityHandle entity);

    static void SelectFile(const std::filesystem::path& filePath);
    static const std::filesystem::path& GetSelectedFile();
    static bool IsFileSelected(const std::filesystem::path& filePath);

    static Type GetSelectionType();
    static void Clear();

private:
    static gns::entityHandle SelectedEntity;
    static std::filesystem::path SelectedFile;
    static Type SelectionType;
};
