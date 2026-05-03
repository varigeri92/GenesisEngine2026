project "Engine"
    kind "SharedLib"
    targetdir(Paths.OutputDir)
    objdir(Obj("Engine"))
    location "Engine"
    pchheader "gnspch.h"
    pchsource "Engine/pch/gnspch.cpp"

    --dependson { "ImGui", "yaml-cpp", "assimp" }
    dependson { "assimp" }

    libdirs {
        LibDir.Assimp,
        LibDir.Output,
        LibDir.Vulkan,
        LibDir.fmt_d
    }

    links {
        Libs.SDL2,
        Libs.SDL2main,
        Libs.Vulkan,
        Libs.Assimp,
        Libs.fmt_d
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

    defines { "BUILD_DLL", "GLM_ENABLE_EXPERIMENTAL", "IMGUI_API=GNS_API", "IMGUI_USER_CONFIG=<imgui_config_helper.h>"}

    includedirs {
        IncludeDir.Vulkan,
        IncludeDir.SDL,
        IncludeDir.Engine_API,
        IncludeDir.Engine_Include,
        IncludeDir.Engine_pch,
        IncludeDir.VKBootstrap,
        IncludeDir.fmt,
        IncludeDir.EnTT,
        IncludeDir.Assimp,
        IncludeDir.Assimp_Build,
        IncludeDir.ImGui,
        IncludeDir.ImGuizmo .. "/src"
        --[[
        IncludeDir.YAML,
        IncludeDir.ImGuizmo
        ]]  
    }

    files { 
        "Engine/**.h",
        "Engine/**.c",
        "Engine/**.cpp",
        "Engine/**.hpp",
        "vendor/vk-bootstrap/src/*.cpp",
        "vendor/vk-bootstrap/src/*.h",
        IncludeDir.ImGui_Backends .. "/imgui_impl_vulkan.h",
        IncludeDir.ImGui_Backends .. "/imgui_impl_vulkan.cpp",
        IncludeDir.ImGui_Backends .. "/imgui_impl_sdl2.h",
        IncludeDir.ImGui_Backends .. "/imgui_impl_sdl2.cpp",

        Submodules.ImGui .. "/*.h",
        Submodules.ImGui .. "/*.cpp",
        IncludeDir.ImGuizmo .. "/src/ImGuizmo.h",
        IncludeDir.ImGuizmo .. "/src/ImGuizmo.cpp",
    }
    filter { 'files:imgui/**.cpp' }
    flags { 'NoPCH' }
        
    filter { 'files:vendor/vk-bootstrap/src/*.cpp' }
    flags { 'NoPCH' }

    filter { 'files:vendor/ImGui/backends/*.cpp' }
    flags { 'NoPCH' }

    filter { 'files:vendor/ImGui/*.cpp' }
    flags { 'NoPCH' }

    filter { 'files:vendor/ImGuizmo/src/ImGuizmo.cpp' }
    defines { "IMGUI_DEFINE_MATH_OPERATORS" }
    flags { 'NoPCH' }

    filter {}
