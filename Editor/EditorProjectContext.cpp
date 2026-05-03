#include "EditorProjectContext.h"

#include "../Engine/Utils/Path.h"

namespace
{
    std::filesystem::path FindPathArgument(
        int argc,
        char** argv,
        const char* shortName,
        const char* longName,
        const std::filesystem::path& fallback)
    {
        for (int index = 1; index < argc; ++index)
        {
            const std::string argument = argv[index] != nullptr ? argv[index] : "";
            if ((argument == shortName || argument == longName) && index + 1 < argc)
            {
                return argv[index + 1];
            }
        }

        return fallback;
    }

    void AddMissingFileError(
        const std::filesystem::path& path,
        const char* label,
        std::vector<std::string>& errors)
    {
        if (gns::path::IsRegularFile(path))
        {
            return;
        }

        errors.emplace_back(std::string("Missing file: ") + label + " (" + path.string() + ")");
    }

    void AddMissingDirectoryError(
        const std::filesystem::path& path,
        const char* label,
        std::vector<std::string>& errors)
    {
        if (gns::path::IsDirectory(path))
        {
            return;
        }

        errors.emplace_back(std::string("Missing folder: ") + label + " (" + path.string() + ")");
    }
}

EditorProjectContext::EditorProjectContext() = default;

std::filesystem::path EditorProjectContext::ProjectRootFromCommandLine(int argc, char** argv)
{
    return gns::path::Normalize(FindPathArgument(
        argc,
        argv,
        "-p",
        "--project",
        gns::path::DefaultProjectDirectory()));
}

std::filesystem::path EditorProjectContext::EditorResourcesRootFromCommandLine(int argc, char** argv)
{
    return gns::path::Normalize(FindPathArgument(
        argc,
        argv,
        "-r",
        "--resources",
        gns::path::DefaultEditorResourcesDirectory()));
}

std::filesystem::path EditorProjectContext::ProjectRoot() const
{
    return gns::path::ProjectDirectory();
}

std::filesystem::path EditorProjectContext::ProjectFilePath() const
{
    return gns::path::ProjectFilePath();
}

std::filesystem::path EditorProjectContext::AssetsPath() const
{
    return gns::path::AssetsDirectory();
}

std::filesystem::path EditorProjectContext::LibraryPath() const
{
    return gns::path::LibraryDirectory();
}

std::filesystem::path EditorProjectContext::PackagesPath() const
{
    return gns::path::PackagesDirectory();
}

std::filesystem::path EditorProjectContext::CachePath() const
{
    return gns::path::CacheDirectory();
}

void EditorProjectContext::Validate()
{
    m_validationErrors.clear();

    AddMissingDirectoryError(ProjectRoot(), "Project root", m_validationErrors);
    AddMissingFileError(ProjectFilePath(), gns::path::ProjectFileName, m_validationErrors);
    AddMissingDirectoryError(AssetsPath(), "Assets", m_validationErrors);
    AddMissingDirectoryError(LibraryPath(), "Library", m_validationErrors);
    AddMissingDirectoryError(PackagesPath(), "Packages", m_validationErrors);
    AddMissingDirectoryError(CachePath(), "Cache", m_validationErrors);
}
