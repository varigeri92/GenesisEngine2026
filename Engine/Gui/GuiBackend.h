#pragma once
#include <cstdint>
#include <functional>
#include <vulkan/vulkan_core.h>

#include "imgui.h"

union SDL_Event;

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
        
        static void DrawImGui(VkCommandBuffer cmd, const VkRenderingInfo& renderInfo);
        static void HandleEvents(SDL_Event& event);
        void OnCreate(SDL_Window* window, gns::RenderSystem& renderSystem);
        void BeginGuiFrame();
        void OnUpdate();
        void OnEndGuiFrame();
        void OnDestroy();
    };
}
