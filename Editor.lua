project "Editor"
    kind "ConsoleApp"
    targetdir(Paths.OutputDir)
    objdir(Obj("Editor"))
    location "Editor"

    libdirs { 
        LibDir.Output,
    }

    links {
        "Engine.lib"
    }
    
    --[[
        filter { "configurations:not Debug" }
        links { Libs.YAMLCPP, Libs.SpirvCrossCore }
        
        filter { "configurations:Debug" }
        links { Libs.YAMLCPPd, Libs.SpirvCrossCore_d }
        
    ]]

    filter {}

    defines { "BUILD_EXE", "GLM_ENABLE_EXPERIMENTAL", "IMGUI_API=GNS_API", "IMGUI_USER_CONFIG=<imgui_config_helper.h>" }

    includedirs {
        IncludeDir.Engine_API,
        IncludeDir.Engine_Include,
        IncludeDir.ImGui,
        IncludeDir.Vulkan,
        IncludeDir.gnsGui
        --[[
            IncludeDir.SDL,
            IncludeDir.Assimp,
            IncludeDir.Assimp_Build,
            IncludeDir.VKBootstrap,
            IncludeDir.EnTT,
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
    
    dependson { "Engine", "ImGui" }
    -- dependson { "Engine", "ImGui", "spirv_reflect", "yaml-cpp", "assimp" }
