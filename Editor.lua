project "Editor"
    kind "ConsoleApp"
    targetdir(Paths.OutputDir)
    objdir(Obj("Editor"))
    location "Editor"

    libdirs { LibDir.Output }

    links { "Engine.lib" }

    filter { "configurations:Release or configurations:Dist" }
        libdirs { LibDir.Assimp_Release, LibDir.YAMLCPP_Release }
        links { Libs.Assimp, Libs.YAMLCPP }

    filter { "configurations:Debug" }
        libdirs { LibDir.Assimp_Debug, LibDir.YAMLCPP_Debug }
        links { Libs.Assimp_d, Libs.YAMLCPPd }

    filter { "configurations:Profile" }
        libdirs { LibDir.Assimp_Debug, LibDir.YAMLCPP_Debug }
        links { Libs.Assimp_d, Libs.YAMLCPPd }

    filter {}

    defines { "BUILD_EXE", "GLM_ENABLE_EXPERIMENTAL", "IMGUI_API=GNS_API", "IMGUI_USER_CONFIG=<imgui_config_helper.h>" }

    includedirs {
        IncludeDir.Engine_API,
        IncludeDir.Engine_Include,
        IncludeDir.ImGui,
        IncludeDir.Vulkan,
        IncludeDir.EnTT,
        IncludeDir.Assimp,
        IncludeDir.Assimp_Build,
        IncludeDir.gnsGui,
        IncludeDir.YAML,
        IncludeDir.ImGuizmo .. "/src"
    }

    files {
        "Editor/**.h",
        "Editor/**.c",
        "Editor/**.cpp",
        "Editor/**.hpp",
        IncludeDir.ImGuizmo .. "/src/ImGuizmo.h"
    }

    -- debugargs { "-p", Paths.ProjectDir, "-r", Paths.ResourceDir }
    
    dependson { "Engine", "ImGui", "yaml-cpp" }
    -- dependson { "Engine", "ImGui", "spirv_reflect", "yaml-cpp", "assimp" }
