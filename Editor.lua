project "Editor"
    kind "ConsoleApp"
    targetdir(Paths.OutputDir)
    objdir(Obj("Editor"))
    location "Editor"

    libdirs { 
        LibDir.Output,
    }

    filter { "system:windows" }
        links {
            "Engine.lib"
        }

        filter { "system:windows", "configurations:not Debug" }
            libdirs { LibDir.YAMLCPP_Release }
            links { Libs.Windows.YAMLCPP }

        filter { "system:windows", "configurations:Debug" }
            libdirs { LibDir.YAMLCPP_Debug }
            links { Libs.Windows.YAMLCPPd }

    filter { "system:linux" }
        libdirs {
            LibDir.Output,
            LibDir.YAMLCPP_Linux
        }
        links {
            "Engine"
        }
        linkoptions { "-lyaml-cpp" }

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
