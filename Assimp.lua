-- assimp.lua
-- Build Assimp via CMake and integrate into Premake

project "assimp"
    kind "Utility"

    -- absolute paths (from premake_cfg.lua)
    local Assimp_SourceDir = Submodules.Assimp
    local Assimp_BuildDir  = Submodules.Assimp .. "/build"

    --
    -- 1. Create build directory
    --
    prebuildcommands {
        ('if not exist "%s" mkdir "%s"'):format(to_win_path(Assimp_BuildDir), to_win_path(Assimp_BuildDir))
    }

    --
    -- 2. Generate Assimp CMake project
    --
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

    --
    -- 3. Build with CMake
    --
    local batch = to_win_path(path.getabsolute("copy_files.bat"))
    local libdir = to_win_path(Assimp_BuildDir .. "/lib")
    local outDir   = to_win_path(path.getabsolute(Paths.OutputDir))
    local bindir = to_win_path(Assimp_BuildDir .. "/bin")

    filter { "configurations:Debug or configurations:Profile" }
        postbuildcommands {
            ('cmake --build "%s" --config Debug'):format(Assimp_BuildDir),
            ('call "%s" "%s" "%s" "Debug"'):format(batch, libdir, outDir),
            ('call "%s" "%s" "%s" "Debug"'):format(batch, bindir, outDir)
        }

        buildoutputs {
            ('%s/lib/Debug/assimp*.lib'):format(Assimp_BuildDir),
            ('%s/bin/Debug/assimp*.dll'):format(Assimp_BuildDir)
        }

    filter { "configurations:Release or configurations:Dist" }
        postbuildcommands {
            ('cmake --build "%s" --config Release'):format(Assimp_BuildDir),
            ('call "%s" "%s" "%s" "Release"'):format(batch, libdir, outDir),
            ('call "%s" "%s" "%s" "Release"'):format(batch, bindir, outDir)
        }

        buildoutputs {
            ('%s/lib/Release/assimp*.lib'):format(Assimp_BuildDir),
            ('%s/bin/Release/assimp*.dll'):format(Assimp_BuildDir)
        }

    filter {}
