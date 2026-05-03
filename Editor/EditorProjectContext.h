#pragma once

#include <filesystem>
#include <string>
#include <vector>

class EditorProjectContext
{
public:
    static constexpr const char* DefaultProjectRoot = R"(D:\ProjectGenesis\TestProject\)";
    static constexpr const char* ProjectFileName = "project.gnsproject";

    EditorProjectContext();
    explicit EditorProjectContext(std::filesystem::path projectRoot);

    static EditorProjectContext FromCommandLine(int argc, char** argv);

    const std::filesystem::path& ProjectRoot() const { return m_projectRoot; }
    const std::filesystem::path& ProjectFilePath() const { return m_projectFilePath; }
    const std::filesystem::path& AssetsPath() const { return m_assetsPath; }
    const std::filesystem::path& LibraryPath() const { return m_libraryPath; }
    const std::filesystem::path& PackagesPath() const { return m_packagesPath; }
    const std::filesystem::path& CachePath() const { return m_cachePath; }
    const std::vector<std::string>& ValidationErrors() const { return m_validationErrors; }

    bool IsValid() const { return m_validationErrors.empty(); }
    void Validate();

private:
    void SetProjectRoot(std::filesystem::path projectRoot);

    std::filesystem::path m_projectRoot;
    std::filesystem::path m_projectFilePath;
    std::filesystem::path m_assetsPath;
    std::filesystem::path m_libraryPath;
    std::filesystem::path m_packagesPath;
    std::filesystem::path m_cachePath;
    std::vector<std::string> m_validationErrors;
};
