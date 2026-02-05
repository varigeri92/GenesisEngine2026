#pragma once
#include <filesystem>
#include <fstream>

namespace gns::fileUtils
{
	inline std::string GetFileExtension(const std::string& path)
    {
        size_t dotPos = path.find_last_of(".");
        if (dotPos != std::string::npos) {
            return path.substr(dotPos + 1);
        }
        return "";
    }

    inline bool HasFileExtension(const std::string& path, const std::string& ext)
    {
        size_t dotPos = path.find_last_of(".");
        if (dotPos != std::string::npos) {
	        return (path.substr(dotPos + 1) == ext);
        }
        return false;
    }

    inline bool FileExists(const std::string& path)
    {
		return std::filesystem::exists(path);
    }

    inline std::string GetFileNameFromPath(const std::string& path)
	{
        std::string base_filename = path.substr(path.find_last_of("/\\") + 1);
        std::string::size_type const p(base_filename.find_last_of('.'));
        std::string file_without_extension = base_filename.substr(0, p);
        return file_without_extension;
	}

    inline void CreateFile(const std::string& path, const std::string& data)
	{
        std::ofstream outfile(path);
        outfile << data << std::endl;
        outfile.close();
	}

    inline std::string ToRelative(const std::string& full_Path, const std::string& root_Path)
	{
        return std::filesystem::relative(full_Path, root_Path).string();
	}

    inline bool IsRootedPath(const std::string& path)
    {
        std::filesystem::path _path = { path };
        return _path.is_absolute();
    }

    inline void DeleteFile(const std::string& path)
	{
        std::remove(path.c_str());
	}

    inline std::string GetContainingDirectory(const std::string& file_path)
	{
        const size_t last_slash_idx = file_path.rfind('\\');
        if (std::string::npos != last_slash_idx)
        {
            return file_path.substr(0, last_slash_idx);
        }
        else
        {
            return "";
        }
	}
}
