#include "gnspch.h"
#include "WindowSystem.h"
#include "Window.h"
#include "../Engine.h"
#include "../API/GenesisWindow.h"
#include "../Systems/SystemsManager.h"

#include <utility>

gns::window::WindowSystem::WindowSystem(
	gns::core::Engine* engine,
	uint32_t width,
	uint32_t height,
	std::string title)
	: m_engine(engine),
	  m_initialWidth(width),
	  m_initialHeight(height),
	  m_title(std::move(title))
{}

void gns::window::WindowSystem::OnCreate()
{
	m_window = std::make_unique<gns::window::Window>(m_initialWidth, m_initialHeight, m_title);
	m_window->Create();
	LOG_INFO("Window system Created!");
}

void gns::window::WindowSystem::OnStart()
{
}

void gns::window::WindowSystem::OnEnable()
{
}

void gns::window::WindowSystem::OnUpdate(float deltaTime)
{
	m_window->PollWindowEvents();
	m_engine->close = ShouldClose();
}

void gns::window::WindowSystem::OnFixedUpdate()
{
}

void gns::window::WindowSystem::OnDisable()
{
}

void gns::window::WindowSystem::OnDestroy()
{
	m_window->CloseWindow();
}

bool gns::window::WindowSystem::ShouldClose()
{
	return m_window->close;
}

SDL_Window* gns::window::WindowSystem::GetSDLWindow()
{
	return m_window->sdl_window;
}

const gns::Screen& gns::window::WindowSystem::GetScreen() const
{
	return m_window->GetScreen();
}

void gns::window::WindowSystem::MinimizeWindow()
{
	if (m_window != nullptr && m_window->sdl_window != nullptr)
	{
		SDL_MinimizeWindow(m_window->sdl_window);
	}
}

void gns::window::WindowSystem::ToggleMaximizeWindow()
{
	if (m_window == nullptr || m_window->sdl_window == nullptr)
	{
		return;
	}

	if (m_manuallyMaximized)
	{
		SDL_SetWindowPosition(m_window->sdl_window, m_restoreX, m_restoreY);
		SDL_SetWindowSize(m_window->sdl_window, m_restoreWidth, m_restoreHeight);
		m_manuallyMaximized = false;
		return;
	}

	SDL_GetWindowPosition(m_window->sdl_window, &m_restoreX, &m_restoreY);
	SDL_GetWindowSize(m_window->sdl_window, &m_restoreWidth, &m_restoreHeight);

	const int displayIndex = SDL_GetWindowDisplayIndex(m_window->sdl_window);
	SDL_Rect displayBounds = {};
	if (displayIndex < 0 || SDL_GetDisplayUsableBounds(displayIndex, &displayBounds) != 0)
	{
		if (SDL_GetDisplayBounds(0, &displayBounds) != 0)
		{
			return;
		}
	}

	SDL_SetWindowPosition(m_window->sdl_window, displayBounds.x, displayBounds.y);
	SDL_SetWindowSize(m_window->sdl_window, displayBounds.w, displayBounds.h);
	m_manuallyMaximized = true;
}

bool gns::window::WindowSystem::IsMaximized() const
{
	return m_manuallyMaximized;
}

void gns::window::WindowSystem::RequestClose()
{
	if (m_window != nullptr)
	{
		m_window->close = true;
	}

	if (m_engine != nullptr)
	{
		m_engine->close = true;
	}
}

void gns::window::WindowSystem::OnLateUpdate(float deltaTime)
{
}

void gns::window::MinimizeMainWindow()
{
	if (WindowSystem* windowSystem = gns::core::SystemsManager::GetSystem<WindowSystem>())
	{
		windowSystem->MinimizeWindow();
	}
}

void gns::window::ToggleMaximizeMainWindow()
{
	if (WindowSystem* windowSystem = gns::core::SystemsManager::GetSystem<WindowSystem>())
	{
		windowSystem->ToggleMaximizeWindow();
	}
}

bool gns::window::IsMainWindowMaximized()
{
	if (WindowSystem* windowSystem = gns::core::SystemsManager::GetSystem<WindowSystem>())
	{
		return windowSystem->IsMaximized();
	}

	return false;
}

void gns::window::RequestCloseMainWindow()
{
	if (WindowSystem* windowSystem = gns::core::SystemsManager::GetSystem<WindowSystem>())
	{
		windowSystem->RequestClose();
	}
}
