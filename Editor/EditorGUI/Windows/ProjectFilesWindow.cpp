#include "ProjectFilesWindow.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include "../../EditorSelection.h"
#include "../../../Engine/Utils/Path.h"


bool IsMetaFile(const std::filesystem::path& path)
{
    return gns::path::HasExtension(path, "meta");
}

std::string GetDisplayName(const std::filesystem::path& path)
{
    const std::string filename = gns::path::FileName(path);
    return filename.empty() ? path.string() : filename;
}

bool IsSameOrChildPath(const std::filesystem::path& path, const std::filesystem::path& parent)
{
    const std::filesystem::path normalizedPath = gns::path::Normalize(path);
    const std::filesystem::path normalizedParent = gns::path::Normalize(parent);
    const std::filesystem::path relativePath = normalizedPath.lexically_relative(normalizedParent);
    if (relativePath.empty())
    {
        return false;
    }

    if (relativePath == ".")
    {
        return true;
    }

    const auto firstPart = relativePath.begin();
    return firstPart != relativePath.end() && *firstPart != "..";
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

        return gns::path::FileName(left.path()) < gns::path::FileName(right.path());
    });

    return children;
}

void DrawFileNode(const std::filesystem::path& filePath)
{
    const std::string id = filePath.string();
    const std::string label = GetDisplayName(filePath);
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_Leaf |
        ImGuiTreeNodeFlags_NoTreePushOnOpen |
        ImGuiTreeNodeFlags_SpanAvailWidth;
    if (EditorSelection::IsFileSelected(filePath))
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    ImGui::PushID(id.c_str());
    ImGui::TreeNodeEx(label.c_str(), flags);
    if (ImGui::IsItemClicked())
    {
        EditorSelection::SelectFile(filePath);
    }
    ImGui::PopID();
}

void ProjectFilesWindow::DrawDirectoryNode(const std::filesystem::path& directory, bool showMetaFiles, bool root)
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
    if (EditorSelection::IsFileSelected(directory))
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (children.empty())
    {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    const std::string id = directory.string();
    ImGui::PushID(id.c_str());
    if (IsSameOrChildPath(m_currentDirectory, directory))
    {
        ImGui::SetNextItemOpen(true, ImGuiCond_Always);
    }
    const bool open = ImGui::TreeNodeEx(label.c_str(), flags);
    if (ImGui::IsItemClicked())
    {
        m_currentDirectory = gns::path::Normalize(directory);
        EditorSelection::SelectFile(m_currentDirectory);
    }
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
            if (m_showFilesInTree)
                DrawFileNode(child.path());
        }

        ImGui::TreePop();
    }
    ImGui::PopID();
}

void ProjectFilesWindow::DrawContentView(const std::filesystem::path& directory, bool showMetaFiles)
{
    std::error_code error;
    const std::vector<std::filesystem::directory_entry> children =
        GetVisibleChildren(directory, showMetaFiles, error);

    if (error)
    {
        ImGui::TextDisabled("Unable to read folder: %s", error.message().c_str());
        return;
    }

    if (children.empty())
    {
        ImGui::TextDisabled("This folder is empty.");
        return;
    }

    ImGuiStyle& style = ImGui::GetStyle();
    const ImVec2 itemSize(160.0f, 120.0f);
    const float windowVisibleX2 = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;

    for (size_t index = 0; index < children.size(); ++index)
    {
        const std::filesystem::directory_entry& child = children[index];
        const std::filesystem::path childPath = gns::path::Normalize(child.path());
        const std::string id = childPath.string();
        const std::string label = GetDisplayName(childPath);

        std::error_code childError;
        const bool isDirectory = child.is_directory(childError);
        const bool isSelected = EditorSelection::IsFileSelected(childPath);

        ImGui::PushID(id.c_str());
        ImGui::BeginChild("item", itemSize, ImGuiChildFlags_Borders);
        const ImVec2 textStart = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("select", ImGui::GetContentRegionAvail());
        const bool clicked = ImGui::IsItemClicked();
        const bool doubleClicked = ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
        ImGui::SetCursorScreenPos(textStart);
        ImGui::TextDisabled("%s", isDirectory ? "Folder" : "File");
        ImGui::Separator();
        ImGui::TextWrapped("%s", label.c_str());
        ImGui::EndChild();

        if (clicked)
        {
            EditorSelection::SelectFile(childPath);
        }
        if (doubleClicked && isDirectory)
        {
            m_currentDirectory = childPath;
            EditorSelection::SelectFile(m_currentDirectory);
        }

        if (isSelected || clicked || doubleClicked)
        {
            const ImU32 selectedColor = ImGui::GetColorU32(ImVec4(0.25f, 0.55f, 1.0f, 1.0f));
            ImGui::GetWindowDrawList()->AddRect(
                ImGui::GetItemRectMin(),
                ImGui::GetItemRectMax(),
                selectedColor,
                0.0f,
                0,
                2.5f);
        }

        const float lastItemX2 = ImGui::GetItemRectMax().x;
        const float nextItemX2 = lastItemX2 + style.ItemSpacing.x + itemSize.x;
        if (index + 1 < children.size() && nextItemX2 < windowVisibleX2)
        {
            ImGui::SameLine();
        }

        ImGui::PopID();
    }
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

void ProjectFilesWindow::OnDraw()
{
    const std::filesystem::path projectRoot = m_projectContext.ProjectRoot();
    if (m_currentDirectory.empty() || !gns::path::IsDirectory(m_currentDirectory))
    {
        m_currentDirectory = gns::path::Normalize(projectRoot);
    }

    ImGui::TextWrapped("%s", m_currentDirectory.string().c_str());
    ImGui::Separator();
    DrawValidationErrors(m_projectContext); 
    ImGui::SameLine();
    ImGui::Checkbox("Show .meta files", &m_showMetaFiles);
    ImGui::SameLine();
    ImGui::Checkbox("Show files in tree view", &m_showFilesInTree);
    ImGui::Separator();

    {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();
        if (!gns::path::IsDirectory(projectRoot))
        {
            ImGui::TextDisabled("Project root cannot be displayed.");
            return;
        }

        const float treeWidth = contentRegionAvail.x * 0.25f;
        ImGui::BeginChild("tree_view", ImVec2(treeWidth, contentRegionAvail.y),
            ImGuiChildFlags_None | ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX, window_flags);
        DrawDirectoryNode(projectRoot, m_showMetaFiles, true);
        ImGui::EndChild();

        ImGui::SameLine();
        const ImVec2 contentRegionAvailRight = ImGui::GetContentRegionAvail();
        ImGui::BeginChild("content_view", ImVec2(contentRegionAvailRight.x, contentRegionAvailRight.y),
            ImGuiChildFlags_None | ImGuiChildFlags_Borders, window_flags);
        DrawContentView(m_currentDirectory, m_showMetaFiles);
        ImGui::EndChild();
    }
}
