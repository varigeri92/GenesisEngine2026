project "Editor"
    kind "ConsoleApp"
    targetdir(Paths.OutputDir)
    objdir(Obj("Editor"))
    location "Editor"

    libdirs { 
        LibDir.Output,
    }

    links {
        "Engine.lib",
    }
    
    --[[
        filter { "configurations:not Debug" }
        links { Libs.YAMLCPP, Libs.SpirvCrossCore }
        
        filter { "configurations:Debug" }
        links { Libs.YAMLCPPd, Libs.SpirvCrossCore_d }
        
    ]]

    filter {}

    defines { "BUILD_EXE", "GLM_ENABLE_EXPERIMENTAL" }

    includedirs {
        IncludeDir.Engine_API,
        IncludeDir.Engine_Include,
        --[[
            IncludeDir.Vulkan,
            IncludeDir.SDL,
            IncludeDir.Assimp,
            IncludeDir.Assimp_Build,
            IncludeDir.VKBootstrap,
            IncludeDir.EnTT,
            IncludeDir.ImGui,
            IncludeDir.ImGuizmo,
            IncludeDir.YAML,
            IncludeDir.SpirvReflect
        ]]
    }

    files {
        "Editor/**.h",
        "Editor/**.c",
        "Editor/**.cpp",
        "Editor/**.hpp"
    }

    -- debugargs { "-p", Paths.ProjectDir, "-r", Paths.ResourceDir }
    
    dependson { "Engine" }
    -- dependson { "Engine", "ImGui", "spirv_reflect", "yaml-cpp", "assimp" }
