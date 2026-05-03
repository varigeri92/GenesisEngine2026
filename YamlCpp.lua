-- yaml-cpp.lua
-- Build yaml-cpp via CMake and integrate into Premake.

project "yaml-cpp"
    kind "Utility"

    local YAMLCPP_SourceDir = Submodules.YAMLCPP
    local YAMLCPP_BuildDir = Submodules.YAMLCPP .. "/build-shared"

    prebuildcommands {
        ('if not exist "%s" mkdir "%s"'):format(to_win_path(YAMLCPP_BuildDir), to_win_path(YAMLCPP_BuildDir))
    }

    prebuildcommands {
        (
            'cmake -S "%s" -B "%s" '
            .. '-DYAML_BUILD_SHARED_LIBS=ON '
            .. '-DYAML_CPP_BUILD_TESTS=OFF '
            .. '-DYAML_CPP_INSTALL=OFF '
        ):format(YAMLCPP_SourceDir, YAMLCPP_BuildDir)
    }

    local batch = to_win_path(path.getabsolute("copy_files.bat"))
    local outDir = to_win_path(path.getabsolute(Paths.OutputDir))

    filter { "configurations:Debug" }
        postbuildcommands {
            ('cmake --build "%s" --config Debug'):format(YAMLCPP_BuildDir),
            ('call "%s" "%s" "%s" "Debug" "*.dll"'):format(batch, to_win_path(YAMLCPP_BuildDir), outDir)
        }

        buildoutputs {
            ('%s/Debug/yaml-cppd.lib'):format(YAMLCPP_BuildDir),
            ('%s/Debug/yaml-cppd.dll'):format(YAMLCPP_BuildDir)
        }

    filter { "configurations:not Debug" }
        postbuildcommands {
            ('cmake --build "%s" --config Release'):format(YAMLCPP_BuildDir),
            ('call "%s" "%s" "%s" "Release" "*.dll"'):format(batch, to_win_path(YAMLCPP_BuildDir), outDir)
        }

        buildoutputs {
            ('%s/Release/yaml-cpp.lib'):format(YAMLCPP_BuildDir),
            ('%s/Release/yaml-cpp.dll'):format(YAMLCPP_BuildDir)
        }

    filter {}
