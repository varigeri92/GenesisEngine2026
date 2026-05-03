#pragma once

#include "Path.h"

#include <string>

namespace gns::fileUtils
{
    inline std::string GetFileExtension(const std::string& path)
    {
        return gns::path::Extension(path);
    }

    inline bool HasFileExtension(const std::string& path, const std::string& ext)
    {
        return gns::path::HasExtension(path, ext);
    }

    inline bool FileExists(const std::string& path)
    {
        return gns::path::Exists(path);
    }

    inline std::string GetFileNameFromPath(const std::string& path)
    {
        return gns::path::FileStem(path);
    }

    inline void CreateFile(const std::string& path, const std::string& data)
    {
        gns::path::WriteTextFile(path, data + "\n");
    }

    inline std::string ToRelative(const std::string& full_Path, const std::string& root_Path)
    {
        return gns::path::ToRelative(full_Path, root_Path).string();
    }

    inline bool IsRootedPath(const std::string& path)
    {
        return gns::path::IsAbsolute(path);
    }

    inline void DeleteFile(const std::string& path)
    {
        gns::path::DeleteFile(path);
    }

    inline std::string GetContainingDirectory(const std::string& file_path)
    {
        return gns::path::ParentDirectory(file_path).string();
    }
}
