#pragma once

#include <filesystem>
#include <string>
#include <utility>

#include "GenesisGUI.h"
#include "../../EditorProjectContext.h"

class ProjectFilesWindow : public GuiWindow
{
public:
    ProjectFilesWindow(std::string title, const EditorProjectContext& projectContext)
        : GuiWindow(std::move(title)), m_projectContext(projectContext)
    {
    }

    void OnDraw() override;
    
private:
    const EditorProjectContext& m_projectContext;
    bool m_showMetaFiles = false;
    bool m_showFilesInTree = false;
    std::filesystem::path m_currentDirectory = {};
    
    void DrawDirectoryNode(const std::filesystem::path& directory, bool showMetaFiles, bool root);
    void DrawContentView(const std::filesystem::path& directory, bool showMetaFiles);

};
