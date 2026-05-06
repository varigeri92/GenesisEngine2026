function to_win_path(p)
    return p:gsub("/", "\\")
end

function is_windows()
    return os.target() == "windows"
end

function is_linux()
    return os.target() == "linux"
end

VULKAN_SDK = os.getenv("VULKAN_SDK")
SolutionRoot = os.getcwd()

local VulkanInclude = "/usr/include"
local VulkanLib = "/usr/lib"
local SDLInclude = "/usr/include/SDL2"

if VULKAN_SDK ~= nil then
    if is_windows() then
        VulkanInclude = VULKAN_SDK .. "/Include"
        VulkanLib = VULKAN_SDK .. "/Lib"
        SDLInclude = VULKAN_SDK .. "/Include/SDL2"
    elseif not VULKAN_SDK:match("^%a:") then
        VulkanInclude = VULKAN_SDK .. "/include"
        VulkanLib = VULKAN_SDK .. "/lib"
    end
end

Paths = {
    Vulkan_Include = VulkanInclude,
    Vulkan_Lib     = VulkanLib,
    SDL_Include    = SDLInclude,
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
    Assimp_Build_Linux = Submodules.Assimp .. "/build-linux/%{cfg.buildcfg}/include",
    
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
    Assimp_Linux = Submodules.Assimp .. "/build-linux/%{cfg.buildcfg}/lib",
    YAMLCPP_Debug = Submodules.YAMLCPP .. "/build-shared/Debug",
    YAMLCPP_Release = Submodules.YAMLCPP .. "/build-shared/Release",
    YAMLCPP_Linux = Submodules.YAMLCPP .. "/build-linux/%{cfg.buildcfg}",
    fmt_d    = Submodules.fmt .. "/build/Debug",
    fmt_r    = Submodules.fmt .. "/build/Release"
}

---------------------------------------
-- THIRD-PARTY LIBS (file names only)
---------------------------------------
Libs = {
    Windows = {
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
    },
    Linux = {
        SDL2             = "SDL2",
        Vulkan           = "vulkan",
        Assimp           = "assimp",
        fmt              = "fmt",
        YAMLCPP          = "yaml-cpp",
        pthread          = "pthread",
        dl               = "dl"
    }
}

Defines = {
    ImGui = {
        "IMGUI_API=GNS_API",
        "IMGUI_USER_CONFIG=\"imgui_config_helper.h\""
    }
}


Out = function(projectName)
    return ("%s/bin/%s/%%{cfg.buildcfg}"):format(SolutionRoot, projectName)
end

Obj = function(projectName)
    return ("%s/bin-int/%s/%%{cfg.buildcfg}"):format(SolutionRoot, projectName)
end
