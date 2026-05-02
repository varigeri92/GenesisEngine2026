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

gns::Reference<gns::Texture> GuiSystem::GetDefaultCheckerboardTexture()
{
    if (m_defaultCheckerboardTexture.m_handle.IsValid())
    {
        return m_defaultCheckerboardTexture;
    }

    gns::RenderSystem* renderSystem = gns::core::SystemsManager::GetSystem<gns::RenderSystem>();
    if (renderSystem == nullptr)
    {
        LOG_WARNING("[GuiSystem]: Cannot get default checkerboard texture because RenderSystem is missing.");
        return {};
    }

    const gns::Handle checkerboardHandle =
        renderSystem->GetDefaultTextureHandle(gns::DefaultTexture::ErrorCheckerboard);
    if (!checkerboardHandle.IsValid())
    {
        LOG_WARNING("[GuiSystem]: Default checkerboard texture handle is invalid.");
        return {};
    }

    m_defaultCheckerboardTexture = gns::Reference<gns::Texture>(checkerboardHandle);
    return m_defaultCheckerboardTexture;
}

uint64_t GuiSystem::GetTextureDescriptor(gns::Reference<gns::Texture> texture) const
{
    if (!texture.m_handle.IsValid())
    {
        LOG_WARNING("[GuiSystem]: Cannot get GUI texture descriptor for invalid texture reference.");
        return 0;
    }

    gns::RenderSystem* renderSystem = gns::core::SystemsManager::GetSystem<gns::RenderSystem>();
    if (renderSystem == nullptr)
    {
        LOG_WARNING("[GuiSystem]: Cannot get GUI texture descriptor because RenderSystem is missing.");
        return 0;
    }

    return renderSystem->GetTextureDescriptor(texture.m_handle);
}

uint64_t GuiSystem::GetSceneTextureDescriptor() const
{
    gns::RenderSystem* renderSystem = gns::core::SystemsManager::GetSystem<gns::RenderSystem>();
    if (renderSystem == nullptr)
    {
        LOG_WARNING("[GuiSystem]: Cannot get scene texture descriptor because RenderSystem is missing.");
        return 0;
    }

    return renderSystem->GetSceneTextureDescriptor();
}

void GuiSystem::SetSceneScreen(const gns::Screen& screen) const
{
    gns::RenderSystem* renderSystem = gns::core::SystemsManager::GetSystem<gns::RenderSystem>();
    if (renderSystem == nullptr)
    {
        LOG_WARNING("[GuiSystem]: Cannot set scene screen because RenderSystem is missing.");
        return;
    }

    renderSystem->SetScreen(screen);
}

void GuiSystem::OnCreate()
{
    gns::window::WindowSystem* ws = 
           gns::core::SystemsManager::GetSystem<gns::window::WindowSystem>();
    if (ws == nullptr)
    {
        LOG_ERROR("[GuiSystem]: WindowSystem is missing. Cannot create GUI backend.");
        return;
    }
    SDL_Window* sdl_window = ws->GetSDLWindow();
    if (sdl_window == nullptr)
    {
        LOG_ERROR("[GuiSystem]: SDL window is missing. Cannot create GUI backend.");
        return;
    }
    gns::RenderSystem* render_system = gns::core::SystemsManager::GetSystem<gns::RenderSystem>();
    if (render_system == nullptr)
    {
        LOG_ERROR("[GuiSystem]: RenderSystem is missing. Cannot create GUI backend.");
        return;
    }
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
    if (render_system == nullptr)
    {
        LOG_WARNING("[GuiSystem]: RenderSystem is missing during GUI shutdown.");
        gui_backend.OnDestroy();
        return;
    }
    render_system->WaitForIdle();
    gui_backend.OnDestroy();
}
