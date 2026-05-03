#include "EditorProjectContext.h"

#include <system_error>
#include <utility>

namespace
{
    std::filesystem::path NormalizePath(const std::filesystem::path& path)
    {
        std::error_code error;
        const std::filesystem::path absolutePath = std::filesystem::absolute(path, error);
        if (error)
        {
            return path.lexically_normal();
        }

        return absolutePath.lexically_normal();
    }

    void AddMissingFileError(
        const std::filesystem::path& path,
        const char* label,
        std::vector<std::string>& errors)
    {
        std::error_code error;
        if (std::filesystem::is_regular_file(path, error))
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
        std::error_code error;
        if (std::filesystem::is_directory(path, error))
        {
            return;
        }

        errors.emplace_back(std::string("Missing folder: ") + label + " (" + path.string() + ")");
    }
}

EditorProjectContext::EditorProjectContext()
    : EditorProjectContext(DefaultProjectRoot)
{
}

EditorProjectContext::EditorProjectContext(std::filesystem::path projectRoot)
{
    SetProjectRoot(std::move(projectRoot));
    Validate();
}

EditorProjectContext EditorProjectContext::FromCommandLine(int argc, char** argv)
{
    std::filesystem::path projectRoot = DefaultProjectRoot;

    for (int index = 1; index < argc; ++index)
    {
        const std::string argument = argv[index] != nullptr ? argv[index] : "";
        if ((argument == "-p" || argument == "--project") && index + 1 < argc)
        {
            projectRoot = argv[index + 1];
            ++index;
        }
    }

    return EditorProjectContext(projectRoot);
}

void EditorProjectContext::Validate()
{
    m_validationErrors.clear();

    AddMissingDirectoryError(m_projectRoot, "Project root", m_validationErrors);
    AddMissingFileError(m_projectFilePath, ProjectFileName, m_validationErrors);
    AddMissingDirectoryError(m_assetsPath, "Assets", m_validationErrors);
    AddMissingDirectoryError(m_libraryPath, "Library", m_validationErrors);
    AddMissingDirectoryError(m_packagesPath, "Packages", m_validationErrors);
    AddMissingDirectoryError(m_cachePath, "Cache", m_validationErrors);
}

void EditorProjectContext::SetProjectRoot(std::filesystem::path projectRoot)
{
    m_projectRoot = NormalizePath(projectRoot);
    m_projectFilePath = m_projectRoot / ProjectFileName;
    m_assetsPath = m_projectRoot / "Assets";
    m_libraryPath = m_projectRoot / "Library";
    m_packagesPath = m_projectRoot / "Packages";
    m_cachePath = m_projectRoot / "Cache";
}
