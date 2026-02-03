include "premake_cfg.lua"

workspace "GenesisEngine"
    language "C++"
    cppdialect "C++20"
    platforms { "Win64" }
    startproject "Editor"
    toolset "msc-v145"
    debugenvs { "TRACEDESIGNTIME = true" }
    
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

group "Core"
    include "Engine.lua"

group "Application"
    include "Editor.lua"
    -- include "Sandbox.lua"
    
group "dependencies"
    --include "ImGui.lua"
    --include "yaml-cpp.lua"
    --include "spirv_reflect.lua"
    --include "Assimp.lua"