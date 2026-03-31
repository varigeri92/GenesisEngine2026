#include "gnspch.h"
#include "GuiSystem.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/RenderSystem.h"
#include "../Window/WindowSystem.h"


GuiSystem::GuiSystem() : System(){}

GuiWindow::GuiWindow(std::string title): open(true), Title(std::move(title)){}

void GuiWindow::BeginWindow()
{
    ImGui::Begin(Title.c_str(), &open);
}

void GuiWindow::EndWindow()
{
    ImGui::End();
}

void GuiSystem::DrawWindows() const
{
    for (auto& window : Windows)
    {
        window->BeginWindow();
        window->OnDraw();
        window->EndWindow();
    }
}

void GuiSystem::OnCreate()
{
    gns::window::WindowSystem* ws = 
           gns::core::SystemsManager::GetSystem<gns::window::WindowSystem>();
    SDL_Window* sdl_window = ws->GetSDLWindow();
    gns::RenderSystem* render_system = gns::core::SystemsManager::GetSystem<gns::RenderSystem>();
    gns::rendering::Renderer& renderer = render_system->GetRenderer();
            
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = renderer.GetInstance();
    init_info.PhysicalDevice = renderer.GetPhysicalDevice();
    init_info.Device = renderer.GetDevice();
    init_info.Queue = renderer.GetGraphicsQueue();
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.UseDynamicRendering = true;
    
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = renderer.GetSwapChainFormat();
    
    gui_backend.OnCreate(sdl_window, init_info);
    LOG_INFO("Gui System Created!");
}

void GuiSystem::OnStart()
{
    LOG_INFO("Gui System Start!");
}

void GuiSystem::OnEnable()
{
    LOG_INFO("Gui System Enable!");
}

void GuiSystem::OnUpdate(float deltaTime)
{
    gui_backend.BeginGuiFrame();
    gui_backend.OnUpdate();
    DrawWindows();
    gui_backend.OnEndGuiFrame();
}

void GuiSystem::OnLateUpdate(float deltaTime)
{
}

void GuiSystem::OnFixedUpdate()
{
}

void GuiSystem::OnDisable()
{
}

void GuiSystem::OnDestroy()
{
    gns::RenderSystem* render_system = gns::core::SystemsManager::GetSystem<gns::RenderSystem>();
    render_system->WaitForIdle();
    gui_backend.OnDestroy();
}
