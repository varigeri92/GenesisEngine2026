#include "ProjectFilesWindow.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace
{
    bool IsMetaFile(const std::filesystem::path& path)
    {
        return path.extension() == ".meta";
    }

    std::string GetDisplayName(const std::filesystem::path& path)
    {
        const std::string filename = path.filename().string();
        return filename.empty() ? path.string() : filename;
    }

    std::vector<std::filesystem::directory_entry> GetVisibleChildren(
        const std::filesystem::path& directory,
        bool showMetaFiles,
        std::error_code& error)
    {
        std::vector<std::filesystem::directory_entry> children;
        const std::filesystem::directory_options options =
            std::filesystem::directory_options::skip_permission_denied;

        std::filesystem::directory_iterator iterator(directory, options, error);
        if (error)
        {
            return children;
        }

        const std::filesystem::directory_iterator end;
        for (; iterator != end; iterator.increment(error))
        {
            if (error)
            {
                break;
            }

            const std::filesystem::directory_entry& entry = *iterator;
            std::error_code entryError;
            if (!showMetaFiles && entry.is_regular_file(entryError) && IsMetaFile(entry.path()))
            {
                continue;
            }

            children.emplace_back(entry);
        }

        std::sort(children.begin(), children.end(), [](const auto& left, const auto& right)
        {
            std::error_code leftError;
            std::error_code rightError;
            const bool leftIsDirectory = left.is_directory(leftError);
            const bool rightIsDirectory = right.is_directory(rightError);
            if (leftIsDirectory != rightIsDirectory)
            {
                return leftIsDirectory;
            }

            return left.path().filename().string() < right.path().filename().string();
        });

        return children;
    }

    void DrawFileNode(const std::filesystem::path& filePath)
    {
        const std::string id = filePath.string();
        const std::string label = GetDisplayName(filePath);
        constexpr ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_Leaf |
            ImGuiTreeNodeFlags_NoTreePushOnOpen |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        ImGui::PushID(id.c_str());
        ImGui::TreeNodeEx(label.c_str(), flags);
        ImGui::PopID();
    }

    void DrawDirectoryNode(const std::filesystem::path& directory, bool showMetaFiles, bool root)
    {
        std::error_code error;
        const std::vector<std::filesystem::directory_entry> children =
            GetVisibleChildren(directory, showMetaFiles, error);

        std::string label = root ? directory.string() : GetDisplayName(directory);
        if (label.empty())
        {
            label = "Project";
        }

        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth;
        if (root)
        {
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }
        if (children.empty())
        {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        const std::string id = directory.string();
        ImGui::PushID(id.c_str());
        const bool open = ImGui::TreeNodeEx(label.c_str(), flags);
        if (error)
        {
            ImGui::TextDisabled("Unable to read folder: %s", error.message().c_str());
        }

        if (open)
        {
            for (const std::filesystem::directory_entry& child : children)
            {
                std::error_code childError;
                if (child.is_directory(childError))
                {
                    DrawDirectoryNode(child.path(), showMetaFiles, false);
                    continue;
                }

                DrawFileNode(child.path());
            }

            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    void DrawValidationErrors(const EditorProjectContext& projectContext)
    {
        if (projectContext.IsValid())
        {
            ImGui::TextDisabled("Project layout is valid.");
            return;
        }

        ImGui::TextColored(ImVec4(0.95f, 0.55f, 0.25f, 1.0f), "Project layout is incomplete.");
        for (const std::string& error : projectContext.ValidationErrors())
        {
            ImGui::BulletText("%s", error.c_str());
        }
    }
}

void ProjectFilesWindow::OnDraw()
{
    ImGui::TextWrapped("Root: %s", m_projectContext.ProjectRoot().string().c_str());
    ImGui::Checkbox("Show .meta files", &m_showMetaFiles);
    ImGui::Separator();

    DrawValidationErrors(m_projectContext);
    ImGui::Separator();

    std::error_code error;
    if (!std::filesystem::is_directory(m_projectContext.ProjectRoot(), error))
    {
        ImGui::TextDisabled("Project root cannot be displayed.");
        return;
    }

    DrawDirectoryNode(m_projectContext.ProjectRoot(), m_showMetaFiles, true);
}
