-- assimp.lua
-- Build Assimp via CMake and integrate into Premake

project "assimp"
    kind "Utility"

    -- absolute paths (from premake_cfg.lua)
    local Assimp_SourceDir = Submodules.Assimp
    local Assimp_BuildDir  = Submodules.Assimp .. "/build"
    local Assimp_LinuxBuildDir = Submodules.Assimp .. "/build-linux/%{cfg.buildcfg}"

    --
    -- 1. Create build directory
    --
    filter { "system:windows" }
        prebuildcommands {
            ('if not exist "%s" mkdir "%s"'):format(to_win_path(Assimp_BuildDir), to_win_path(Assimp_BuildDir))
        }

        prebuildcommands {
            (
                'cmake -S "%s" -B "%s" '
                .. '-DASSIMP_BUILD_TESTS=OFF '
                .. '-DASSIMP_BUILD_SAMPLES=OFF '
                .. '-DASSIMP_BUILD_FBX_IMPORTER=ON '
                .. '-DASSIMP_BUILD_SHARED_LIBS=ON '
                .. '-DASSIMP_INJECT_DEBUG_POSTFIX=ON '
            ):format(Assimp_SourceDir, Assimp_BuildDir)
        }

        postbuildcommands {
            ('cmake --build "%s" --config Release'):format(Assimp_BuildDir)
        }

        local batch = to_win_path(path.getabsolute("copy_files.bat"))
        local libdir = to_win_path(Assimp_BuildDir .. "/lib")
        local outDir = to_win_path(path.getabsolute(Paths.OutputDir))
        postbuildcommands {
            ('call "%s" "%s" "%s" "Release"'):format(batch, libdir, outDir)
        }

        local bindir = to_win_path(Assimp_BuildDir .. "/bin")
        postbuildcommands {
            ('call "%s" "%s" "%s" "Release"'):format(batch, bindir, outDir)
        }

        buildoutputs {
            ('%s/lib/Release/assimp*.lib'):format(Assimp_BuildDir),
            ('%s/bin/Release/assimp*.dll'):format(Assimp_BuildDir)
        }

    filter { "system:linux" }
        prebuildcommands {
            ('mkdir -p "%s"'):format(Assimp_LinuxBuildDir),
            (
                'cmake -S "%s" -B "%s" -DCMAKE_BUILD_TYPE=%%{cfg.buildcfg} '
                .. '-DASSIMP_BUILD_TESTS=OFF '
                .. '-DASSIMP_BUILD_SAMPLES=OFF '
                .. '-DASSIMP_BUILD_FBX_IMPORTER=ON '
                .. '-DASSIMP_BUILD_SHARED_LIBS=ON '
            ):format(Assimp_SourceDir, Assimp_LinuxBuildDir)
        }

        postbuildcommands {
            ('cmake --build "%s"'):format(Assimp_LinuxBuildDir),
            ('mkdir -p "%s"'):format(path.getabsolute(Paths.OutputDir)),
            ('cp "%s"/lib/libassimp.so* "%s"/'):format(Assimp_LinuxBuildDir, path.getabsolute(Paths.OutputDir))
        }

        buildoutputs {
            ('%s/lib/libassimp.so'):format(Assimp_LinuxBuildDir)
        }

    filter {}
