function to_win_path(p)
    return p:gsub("/", "\\")
end

VULKAN_SDK = os.getenv("VULKAN_SDK")
SolutionRoot = os.getcwd()

Paths = {
    Vulkan_Include = VULKAN_SDK .. "/Include",
    Vulkan_Lib     = VULKAN_SDK .. "/Lib",
    SDL_Include    = VULKAN_SDK .. "/Include/SDL2",
    OutputDir      = "bin/%{cfg.buildcfg}",
    IntermediateDir= "bin-int"
}

---------------------------------------
-- SUBMODULE ROOTS
---------------------------------------
Submodules = {
    fmt          = path.getabsolute("vendor/fmt"),
    Assimp       = path.getabsolute("vendor/assimp"),
    YAMLCPP      = path.getabsolute("vendor/yaml-cpp"),
    ImGui        = path.getabsolute("vendor/ImGui"),
    ImGuizmo     = path.getabsolute("vendor/ImGuizmo"),
    VKBootstrap  = path.getabsolute("vendor/vk-bootstrap"),
    EnTT         = path.getabsolute("vendor/entt"),
    SpirvReflect = path.getabsolute("vendor/spirv_reflect"),
}

---------------------------------------
-- SUBMODULE INCLUDE DIRECTORIES
---------------------------------------
IncludeDir = {
    Vulkan        = Paths.Vulkan_Include,
    SDL           = Paths.SDL_Include,
    fmt           = Submodules.fmt .. "/include",
    -- Engine
    Engine_API     = "Engine/API",
    Engine_Include = "Engine/include",
    Engine_pch = "Engine/pch",
    -- Submodules
    ImGui          = Submodules.ImGui,
    ImGui_Backends = Submodules.ImGui .. "/backends",
    ImGuizmo       = Submodules.ImGuizmo,
    gnsGui         = "ImGui",
    
    Assimp         = Submodules.Assimp .. "/include",
    Assimp_Build   = Submodules.Assimp .. "/build/include",
    
    VKBootstrap    = Submodules.VKBootstrap .. "/src",
    EnTT           = Submodules.EnTT .. "/single_include",
    YAML           = Submodules.YAMLCPP .. "/include",
    SpirvReflect   = Submodules.SpirvReflect
}

---------------------------------------
-- LIBRARY DIRECTORIES
---------------------------------------
LibDir = {
    Output   = Paths.OutputDir,
    Vulkan   = Paths.Vulkan_Lib,
    Assimp   = Submodules.Assimp .. "/build/lib/Release",
    fmt_d    = Submodules.fmt .. "/build/Debug",
    fmt_r    = Submodules.fmt .. "/build/Release"
}

---------------------------------------
-- THIRD-PARTY STATIC LIBS (file names only)
---------------------------------------
Libs = {
    SDL2             = "SDL2.lib",
    SDL2main         = "SDL2main.lib",
    Vulkan           = "vulkan-1.lib",
    Assimp           = "assimp-vc145-mt.lib",
    Assimp_d         = "assimp-vc145-mtd.lib",
    fmt              = "fmt.lib",
    fmt_d            = "fmtd.lib",
    ImGui            = "ImGui.lib",
    SpirvReflect     = "spirv_reflect.lib",
    YAMLCPPd         = "yaml-cppd.lib",
    YAMLCPP          = "yaml-cpp.lib",
    SpirvCrossCore_d = "spirv-cross-cored.lib",
    SpirvCrossCore   = "spirv-cross-core.lib"
}


Out = function(projectName)
    return ("%s/bin/%s/%%{cfg.buildcfg}"):format(SolutionRoot, projectName)
end

Obj = function(projectName)
    return ("%s/bin-int/%s/%%{cfg.buildcfg}"):format(SolutionRoot, projectName)
end
