project "Engine"
    kind "SharedLib"
    targetdir(Paths.OutputDir)
    objdir(Obj("Engine"))
    location "Engine"
    pchheader "gnspch.h"
    pchsource "Engine/pch/gnspch.cpp"

    -- dependson { "ImGui", "yaml-cpp", "assimp" }

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
        --[[ 
            Libs.Assimp,
            ]]
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

    defines { "BUILD_DLL", "GLM_ENABLE_EXPERIMENTAL" }

    includedirs {
        IncludeDir.Vulkan,
        IncludeDir.SDL,
        IncludeDir.Engine_API,
        IncludeDir.Engine_Include,
        IncludeDir.Engine_pch,
        IncludeDir.VKBootstrap,
        IncludeDir.fmt,
        IncludeDir.ImGui
        --[[
        IncludeDir.Assimp,
        IncludeDir.Assimp_Build,
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
        "vendor/vk-bootstrap/src/*.cpp",
        "vendor/vk-bootstrap/src/*.h",
        IncludeDir.ImGui_Backends .. "/imgui_impl_vulkan.h",
        IncludeDir.ImGui_Backends .. "/imgui_impl_vulkan.cpp",
        IncludeDir.ImGui_Backends .. "/imgui_impl_sdl2.h",
        IncludeDir.ImGui_Backends .. "/imgui_impl_sdl2.cpp",

        Submodules.ImGui .. "/*.h",
        Submodules.ImGui .. "/*.cpp",
    }
    filter { 'files:imgui/**.cpp' }
    flags { 'NoPCH' }
        
    filter { 'files:vendor/vk-bootstrap/src/*.cpp' }
    flags { 'NoPCH' }

    filter { 'files:vendor/ImGui/backends/*.cpp' }
    flags { 'NoPCH' }

    filter { 'files:vendor/ImGui/*.cpp' }
    flags { 'NoPCH' }