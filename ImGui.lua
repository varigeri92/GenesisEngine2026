-- projects/ImGui.lua

project "ImGui"
    kind "StaticLib"
    targetdir(LibDir.Output)
    objdir(Obj("ImGui"))
    location "ImGui"
    
    libdirs {
        LibDir.Assimp,
        LibDir.Output,
        LibDir.Vulkan
    }
    
    links {
        "Engine.lib",
        Libs.SDL2,
        Libs.Vulkan
    }

    defines { "BUILD_ENGINE_LIB" }

    includedirs {
        IncludeDir.Vulkan,
        IncludeDir.SDL,
        IncludeDir.ImGui
    }

    files {
        -- ImGui core + backends
        IncludeDir.ImGui_Backends .. "/imgui_impl_vulkan.h",
        IncludeDir.ImGui_Backends .. "/imgui_impl_vulkan.cpp",
        IncludeDir.ImGui_Backends .. "/imgui_impl_sdl2.h",
        IncludeDir.ImGui_Backends .. "/imgui_impl_sdl2.cpp",

        Submodules.ImGui .. "/*.h",
        Submodules.ImGui .. "/*.cpp",

        "ImGui/**.h",
        "ImGui/**.c",
        "ImGui/**.cpp",
        "ImGui/**.hpp",   
        -- ImGuizmo
        -- Submodules.ImGuizmo .. "/ImGuizmo.cpp",
        -- Submodules.ImGuizmo .. "/ImGuizmo.h"
    }
