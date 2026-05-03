#include "gnspch.h"
#include "Path.h"

#include <fstream>
#include <system_error>

namespace
{
    struct PathState
    {
        std::filesystem::path projectRoot;
        std::filesystem::path editorResourcesRoot;
    };

    PathState g_pathState = {};

    std::filesystem::path FindAncestorPath(
        const std::filesystem::path& start,
        const std::filesystem::path& relativePath)
    {
        std::filesystem::path current = gns::path::Normalize(start);
        while (!current.empty())
        {
            const std::filesystem::path candidate = current / relativePath;
            if (gns::path::Exists(candidate))
            {
                return gns::path::Normalize(candidate);
            }

            const std::filesystem::path parent = current.parent_path();
            if (parent == current)
            {
                break;
            }

            current = parent;
        }

        return {};
    }
}

std::filesystem::path gns::path::DefaultProjectDirectory()
{
    return Normalize(DefaultProjectRoot);
}

std::filesystem::path gns::path::DefaultEditorResourcesDirectory()
{
    std::error_code error;
    const std::filesystem::path current = std::filesystem::current_path(error);
    if (!error)
    {
        const std::filesystem::path discovered = FindAncestorPath(current, "Resources");
        if (!discovered.empty())
        {
            return discovered;
        }

        return Normalize(current.parent_path() / "Resources");
    }

    return Normalize("Resources");
}

void gns::path::Configure(
    const std::filesystem::path& projectRoot,
    const std::filesystem::path& editorResourcesRoot)
{
    if (!projectRoot.empty())
    {
        SetProjectDirectory(projectRoot);
    }

    if (!editorResourcesRoot.empty())
    {
        SetEditorResourcesDirectory(editorResourcesRoot);
    }
    else if (g_pathState.editorResourcesRoot.empty())
    {
        SetEditorResourcesDirectory(DefaultEditorResourcesDirectory());
    }
}

void gns::path::SetProjectDirectory(const std::filesystem::path& projectRoot)
{
    g_pathState.projectRoot = Normalize(projectRoot);
    LOG_INFO("[Path]: Project root: " + g_pathState.projectRoot.string());
}

void gns::path::SetEditorResourcesDirectory(const std::filesystem::path& editorResourcesRoot)
{
    g_pathState.editorResourcesRoot = Normalize(editorResourcesRoot);
    LOG_INFO("[Path]: Editor resources root: " + g_pathState.editorResourcesRoot.string());
}

std::filesystem::path gns::path::RootDirectory(Root root)
{
    switch (root)
    {
    case Root::Project:
        return ProjectDirectory();
    case Root::ProjectAssets:
        return AssetsDirectory();
    case Root::ProjectLibrary:
        return LibraryDirectory();
    case Root::ProjectPackages:
        return PackagesDirectory();
    case Root::ProjectCache:
        return CacheDirectory();
    case Root::EditorResources:
        return EditorResourcesDirectory();
    default:
        return {};
    }
}

std::filesystem::path gns::path::Resolve(Root root, const std::filesystem::path& relativePath)
{
    return ResolveAgainst(RootDirectory(root), relativePath);
}

std::filesystem::path gns::path::ResolveAgainst(
    const std::filesystem::path& root,
    const std::filesystem::path& path)
{
    if (path.is_absolute())
    {
        return Normalize(path);
    }

    return Normalize(root / path);
}

std::filesystem::path gns::path::ProjectDirectory()
{
    return g_pathState.projectRoot;
}

std::filesystem::path gns::path::ProjectFilePath()
{
    return Resolve(Root::Project, ProjectFileName);
}

std::filesystem::path gns::path::AssetsDirectory()
{
    return Resolve(Root::Project, "Assets");
}

std::filesystem::path gns::path::LibraryDirectory()
{
    return Resolve(Root::Project, "Library");
}

std::filesystem::path gns::path::PackagesDirectory()
{
    return Resolve(Root::Project, "Packages");
}

std::filesystem::path gns::path::CacheDirectory()
{
    return Resolve(Root::Project, "Cache");
}

std::filesystem::path gns::path::EditorResourcesDirectory()
{
    return g_pathState.editorResourcesRoot;
}

std::filesystem::path gns::path::ResourcesDirectory()
{
    return EditorResourcesDirectory();
}

std::filesystem::path gns::path::InResourcesDirectory(const std::filesystem::path& relativePath)
{
    return Resolve(Root::EditorResources, relativePath);
}

void gns::path::SetResourcesDirectory()
{
    SetEditorResourcesDirectory(DefaultEditorResourcesDirectory());
}

std::filesystem::path gns::path::Normalize(const std::filesystem::path& path)
{
    std::error_code error;
    const std::filesystem::path absolutePath = std::filesystem::absolute(path, error);
    if (error)
    {
        return path.lexically_normal();
    }

    return absolutePath.lexically_normal();
}

std::filesystem::path gns::path::ToRelative(
    const std::filesystem::path& path,
    const std::filesystem::path& root)
{
    std::error_code error;
    const std::filesystem::path relativePath = std::filesystem::relative(path, root, error);
    if (error)
    {
        return path.lexically_normal();
    }

    return relativePath.lexically_normal();
}

std::filesystem::path gns::path::ParentDirectory(const std::filesystem::path& path)
{
    return path.parent_path();
}

std::string gns::path::Extension(const std::filesystem::path& path)
{
    std::string extension = path.extension().string();
    if (!extension.empty() && extension[0] == '.')
    {
        extension.erase(0, 1);
    }

    return extension;
}

bool gns::path::HasExtension(const std::filesystem::path& path, const std::string& extension)
{
    std::string normalizedExtension = extension;
    if (!normalizedExtension.empty() && normalizedExtension[0] == '.')
    {
        normalizedExtension.erase(0, 1);
    }

    return Extension(path) == normalizedExtension;
}

std::string gns::path::FileName(const std::filesystem::path& path)
{
    return path.filename().string();
}

std::string gns::path::FileStem(const std::filesystem::path& path)
{
    return path.stem().string();
}

bool gns::path::Exists(const std::filesystem::path& path)
{
    std::error_code error;
    return std::filesystem::exists(path, error);
}

bool gns::path::IsRegularFile(const std::filesystem::path& path)
{
    std::error_code error;
    return std::filesystem::is_regular_file(path, error);
}

bool gns::path::IsDirectory(const std::filesystem::path& path)
{
    std::error_code error;
    return std::filesystem::is_directory(path, error);
}

bool gns::path::IsAbsolute(const std::filesystem::path& path)
{
    return path.is_absolute();
}

bool gns::path::IsSameOrChildPath(
    const std::filesystem::path& path,
    const std::filesystem::path& parent)
{
    const std::filesystem::path normalizedPath = Normalize(path);
    const std::filesystem::path normalizedParent = Normalize(parent);
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

bool gns::path::WriteTextFile(const std::filesystem::path& path, const std::string& data)
{
    std::ofstream file(path);
    if (!file)
    {
        return false;
    }

    file << data;
    return true;
}

bool gns::path::DeleteFile(const std::filesystem::path& path)
{
    std::error_code error;
    return std::filesystem::remove(path, error);
}
