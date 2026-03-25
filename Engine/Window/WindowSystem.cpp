#include "gnspch.h"
#include "WindowSystem.h"
#include "Window.h"
#include "../Engine.h"

gns::window::WindowSystem::WindowSystem(gns::core::Engine* engine): m_engine(engine)
{}

void gns::window::WindowSystem::OnCreate()
{
	m_window = std::make_unique<gns::window::Window>(1920, 1080, "GenesisTestWindow");
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

void gns::window::WindowSystem::OnLateUpdate(float deltaTime)
{
}
