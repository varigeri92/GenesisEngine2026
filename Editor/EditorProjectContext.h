#pragma once

#include <filesystem>
#include <string>
#include <vector>

class EditorProjectContext
{
public:
    EditorProjectContext();

    static std::filesystem::path ProjectRootFromCommandLine(int argc, char** argv);
    static std::filesystem::path EditorResourcesRootFromCommandLine(int argc, char** argv);

    std::filesystem::path ProjectRoot() const;
    std::filesystem::path ProjectFilePath() const;
    std::filesystem::path AssetsPath() const;
    std::filesystem::path LibraryPath() const;
    std::filesystem::path PackagesPath() const;
    std::filesystem::path CachePath() const;
    const std::vector<std::string>& ValidationErrors() const { return m_validationErrors; }

    bool IsValid() const { return m_validationErrors.empty(); }
    void Validate();

private:
    std::vector<std::string> m_validationErrors;
};
