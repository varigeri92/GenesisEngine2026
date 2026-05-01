#include "gnspch.h"
#include "GuiSystem.h"
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

uint64_t GuiSystem::GetDefaultCheckerboardTexture()
{
    if (m_defaultCheckerboardTexture != 0)
    {
        return m_defaultCheckerboardTexture;
    }

    gns::RenderSystem* renderSystem = gns::core::SystemsManager::GetSystem<gns::RenderSystem>();
    if (renderSystem == nullptr)
    {
        return 0;
    }

    const gns::Handle checkerboardHandle =
        renderSystem->GetDefaultTextureHandle(gns::DefaultTexture::ErrorCheckerboard);
    if (!checkerboardHandle.IsValid())
    {
        return 0;
    }

    m_defaultCheckerboardTexture = gui_backend.RegisterTexture(*renderSystem, checkerboardHandle);
    return m_defaultCheckerboardTexture;
}

void GuiSystem::OnCreate()
{
    gns::window::WindowSystem* ws = 
           gns::core::SystemsManager::GetSystem<gns::window::WindowSystem>();
    SDL_Window* sdl_window = ws->GetSDLWindow();
    gns::RenderSystem* render_system = gns::core::SystemsManager::GetSystem<gns::RenderSystem>();
    gui_backend.OnCreate(sdl_window, *render_system);
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
