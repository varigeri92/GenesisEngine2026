#include "ProjectFilesModel.h"

#include <algorithm>
#include <cctype>

#include "../../Engine/Utils/Path.h"

bool editor::projectfiles::IsMetaFile(const std::filesystem::path& path)
{
    return gns::path::HasExtension(path, "meta");
}

gns::assets::AssetType editor::projectfiles::GetAssetTypeFromPath(const std::filesystem::path& path)
{
    std::string extension = gns::path::Extension(path);
    for (char& c : extension)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    if (extension == "gltf" || extension == "glb" || extension == "obj" ||
        extension == "fbx" || extension == "dae" || extension == "3ds" ||
        extension == "blend" || extension == "ply" || extension == "stl")
    {
        return gns::assets::Mesh;
    }

    if (extension == "png" || extension == "jpg" || extension == "jpeg" ||
        extension == "tga" || extension == "bmp")
    {
        return gns::assets::Texture;
    }

    if (extension == "comp")
    {
        return gns::assets::ComputeShader;
    }

    if (extension == "vert" || extension == "frag" || extension == "glsl")
    {
        return gns::assets::Shader;
    }

    return gns::assets::Generic;
}

std::string editor::projectfiles::GetDisplayName(const std::filesystem::path& path)
{
    const std::string filename = gns::path::FileName(path);
    return filename.empty() ? path.string() : filename;
}

std::vector<editor::projectfiles::ProjectFileEntry> editor::projectfiles::GetVisibleChildren(
    const std::filesystem::path& directory,
    bool showMetaFiles,
    std::error_code& error)
{
    std::vector<ProjectFileEntry> children;
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
        std::error_code regularFileError;
        const bool isRegularFile = entry.is_regular_file(regularFileError);
        if (!showMetaFiles && isRegularFile && IsMetaFile(entry.path()))
        {
            continue;
        }

        std::error_code directoryError;
        const std::filesystem::path entryPath = gns::path::Normalize(entry.path());
        children.emplace_back(ProjectFileEntry
        {
            .path = entryPath,
            .displayName = GetDisplayName(entryPath),
            .isDirectory = entry.is_directory(directoryError),
            .isRegularFile = isRegularFile
        });
    }

    std::sort(children.begin(), children.end(), [](const ProjectFileEntry& left, const ProjectFileEntry& right)
    {
        if (left.isDirectory != right.isDirectory)
        {
            return left.isDirectory;
        }

        return left.displayName < right.displayName;
    });

    return children;
}
