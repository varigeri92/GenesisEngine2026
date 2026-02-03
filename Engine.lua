project "Engine"
    kind "SharedLib"
    targetdir(Paths.OutputDir)
    objdir(Obj("Engine"))
    location "Engine"

    pchheader "gnspch.h"
    pchsource "Engine/pch/gnspch.cpp"

    configurations { "Debug", "Release", "Profile", "Dist" }
        filter { "platforms:Win64" }
            system "Windows"
            architecture "x86_64"

        filter "configurations:Debug"
            defines { "DEBUG", "_DEBUG", "_CONSOLE", "LOG_ENABLE" }
            symbols "On"

        filter "configurations:Profile"
            defines { "DEBUG", "_DEBUG", "_CONSOLE", "LOG_ENABLE", "ENABLE_PROFILER", "TRACE_ALLOCATION" }
            symbols "On"

        filter "configurations:Release"
            defines { "NDEBUG", "LOG_ENABLE" }
            optimize "On"

        filter "configurations:Dist"
            defines { "NDEBUG" }
            optimize "On"


    -- dependson { "ImGui", "yaml-cpp", "assimp" }

    libdirs {
        LibDir.Assimp,
        LibDir.Output,
        LibDir.Vulkan
    }

    links {
        Libs.SDL2,
        Libs.SDL2main,
        Libs.Vulkan,
        --[[ 
        Libs.Assimp,
        Libs.ImGui,
        ]]
    }
    
    
    --[[
    -- Release library
    filter { "configurations:not Debug" }
    links { Libs.YAMLCPP }
    
    -- Debug-only library
    filter { "configurations:Debug" }
    links { Libs.YAMLCPPd }
    ]]
    filter {}

    defines { "BUILD_DLL", "GLM_ENABLE_EXPERIMENTAL" }

    includedirs {
        IncludeDir.Vulkan,
        IncludeDir.SDL,
        IncludeDir.Engine_API,
        IncludeDir.Engine_Include,
        IncludeDir.Engine_pch
      --[[
        IncludeDir.ImGui,
        IncludeDir.Assimp,
        IncludeDir.Assimp_Build,
        IncludeDir.VKBootstrap,
        IncludeDir.EnTT,
        IncludeDir.YAML,
        IncludeDir.ImGuizmo
        ]]  
    }

    files { 
        "Engine/**.h",
        "Engine/**.c",
        "Engine/**.cpp",
        "Engine/**.hpp",

        --[[
            -- vk-bootstrap
            "submodules/vk-bootstrap/src/*.cpp",
            "submodules/vk-bootstrap/src/*.h"
        ]]
        
    }
    --[[
        filter { 'files:imgui/**.cpp' }
        flags { 'NoPCH' }
        
        filter { 'files:submodules/vk-bootstrap/src/*.cpp' }
        flags { 'NoPCH' }
    ]]