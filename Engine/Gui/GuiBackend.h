#pragma once
#include <functional>
#include <vulkan/vulkan_core.h>

#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_sdl2.h"

namespace gns
{
    namespace window
    {
        class WindowSystem;
    }

    class RenderSystem;
}

struct SDL_Window;

namespace gns::gui
{
    class GuiBackend
    {
        VkDescriptorPool m_imguiPool = VK_NULL_HANDLE;
        SDL_Window* m_window = nullptr;
        VkDevice m_device = VK_NULL_HANDLE;
    public:
        GuiBackend();
        ~GuiBackend();
        
        static void DrawImGui(VkCommandBuffer cmd, const VkRenderingInfo& renderInfo );
        static void HandleEvents(SDL_Event& event);
        void OnCreate(SDL_Window* window, ImGui_ImplVulkan_InitInfo& init_info);
        void BeginGuiFrame();
        void OnUpdate();
        void OnEndGuiFrame();
        void OnDestroy();
    };
}
