#pragma once

#include "../API/API.h"

#include <filesystem>
#include <string>

namespace gns::path
{
    #if defined(_WIN32)
    inline constexpr const char* DefaultProjectRoot = R"(D:\ProjectGenesis\TestProject\)";
    #else
    inline constexpr const char* DefaultProjectRoot = "TestProject";
    #endif
    inline constexpr const char* ProjectFileName = "project.gnsproject";

    enum class Root
    {
        Project,
        ProjectAssets,
        ProjectLibrary,
        ProjectPackages,
        ProjectCache,
        EditorResources
    };

    GNS_API std::filesystem::path DefaultProjectDirectory();
    GNS_API std::filesystem::path DefaultEditorResourcesDirectory();

    GNS_API void Configure(
        const std::filesystem::path& projectRoot,
        const std::filesystem::path& editorResourcesRoot = {});
    GNS_API void SetProjectDirectory(const std::filesystem::path& projectRoot);
    GNS_API void SetEditorResourcesDirectory(const std::filesystem::path& editorResourcesRoot);

    GNS_API std::filesystem::path RootDirectory(Root root);
    GNS_API std::filesystem::path Resolve(Root root, const std::filesystem::path& relativePath);
    GNS_API std::filesystem::path ResolveAgainst(
        const std::filesystem::path& root,
        const std::filesystem::path& path);

    GNS_API std::filesystem::path ProjectDirectory();
    GNS_API std::filesystem::path ProjectFilePath();
    GNS_API std::filesystem::path AssetsDirectory();
    GNS_API std::filesystem::path LibraryDirectory();
    GNS_API std::filesystem::path PackagesDirectory();
    GNS_API std::filesystem::path CacheDirectory();
    GNS_API std::filesystem::path EditorResourcesDirectory();

    GNS_API std::filesystem::path ResourcesDirectory();
    GNS_API std::filesystem::path InResourcesDirectory(const std::filesystem::path& relativePath);
    GNS_API void SetResourcesDirectory();

    GNS_API std::filesystem::path Normalize(const std::filesystem::path& path);
    GNS_API std::filesystem::path ToRelative(
        const std::filesystem::path& path,
        const std::filesystem::path& root);
    GNS_API std::filesystem::path ParentDirectory(const std::filesystem::path& path);

    GNS_API std::string Extension(const std::filesystem::path& path);
    GNS_API bool HasExtension(const std::filesystem::path& path, const std::string& extension);
    GNS_API std::string FileName(const std::filesystem::path& path);
    GNS_API std::string FileStem(const std::filesystem::path& path);

    GNS_API bool Exists(const std::filesystem::path& path);
    GNS_API bool IsRegularFile(const std::filesystem::path& path);
    GNS_API bool IsDirectory(const std::filesystem::path& path);
    GNS_API bool IsAbsolute(const std::filesystem::path& path);
    GNS_API bool IsSameOrChildPath(
        const std::filesystem::path& path,
        const std::filesystem::path& parent);

    GNS_API bool WriteTextFile(const std::filesystem::path& path, const std::string& data);
    GNS_API bool DeleteFile(const std::filesystem::path& path);
}
