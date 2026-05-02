#include "gnspch.h"
#include "Engine.h"

#include "Core/Entity.h"
#include "Gui/TestWindow.h"
#include "Log/Logger.h"
#include "Systems/SystemsManager.h"
#include "Systems/TestSystem.h"
#include "Window/WindowSystem.h"
#include "Utils/Time.h"
#include "Renderer/RenderSystem.h"
#include "Scene/SceneSystem.h"
#include "Systems/GuiSystem.h"
#include "Utils/Path.h"

gns::core::Engine::Engine(EngineConfig cfg) : close(false), m_engineConfig{cfg.headless, cfg.InitTetsSystem} {}


void gns::core::Engine::Initialize(std::function<void()> callback)
{
	gns::path::SetResourcesDirectory();
	SystemsManager::RegisterSystem<gns::SceneSystem>();
	gns::window::WindowSystem* ws = nullptr;
	GuiSystem* gui_system = nullptr;
	if (!m_engineConfig.headless) {
		ws = SystemsManager::RegisterSystem<gns::window::WindowSystem>(this);
		SystemsManager::RegisterSystem<gns::RenderSystem>(ws);
		gui_system = gns::core::SystemsManager::RegisterSystem<GuiSystem>();
	}
	
	
	if(m_engineConfig.InitTetsSystem)
		SystemsManager::RegisterSystem<TestSystem>();
	callback();
	
	if (gui_system!=nullptr)
	{
		gui_system->RegisterWindow<TestWindow>("TestWindow");
	}
}

void gns::core::Engine::Run()
{
	while (!close)
	{
		gns::Time::StartFrameTime();
		float deltaTime = gns::Time::DeltaTime();
		SystemsManager::Run(deltaTime);
		gns::Time::EndFrameTime();
	}
}

void gns::core::Engine::ShutDown()
{
	SystemsManager::Clear();
}

SDL_Window* gns::core::Engine::GetWindow()
{
	window::WindowSystem* ws = SystemsManager::GetSystem<window::WindowSystem>();
	return ws->GetSDLWindow();
}

gns::rendering::Renderer& gns::core::Engine::GetRenderer()
{
	RenderSystem* rs = SystemsManager::GetSystem<RenderSystem>();
	return rs->GetRenderer();
}
