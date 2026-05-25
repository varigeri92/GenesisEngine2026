#include "gnspch.h"
#include "Engine.h"

#include "Core/ComponentLibrary.h"
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
#include "Systems/TransformSystem.h"
#include "Utils/Path.h"

#include <utility>

#include "JobSystem/JobSystem.h"
#include "JobSystem/TestJob.h"

gns::core::Engine::Engine(EngineConfig cfg) : close(false), m_engineConfig{std::move(cfg)} {}


void gns::core::Engine::Initialize(std::function<void()> callback)
{
	GNS_PROFILE_FUNCTION();
	gns::jobs::JobSystem::Initialize();
	gns::path::Configure(m_engineConfig.projectRoot, m_engineConfig.editorResourcesRoot);
	gns::RegisterCoreComponentReflection();
	SystemsManager::RegisterSystem<gns::SceneSystem>();
	SystemsManager::RegisterSystem<TransformSystem>();
	gns::window::WindowSystem* ws = nullptr;
	GuiSystem* gui_system = nullptr;
	if (!m_engineConfig.headless) {
		ws = SystemsManager::RegisterSystem<gns::window::WindowSystem>(
			this,
			m_engineConfig.windowWidth,
			m_engineConfig.windowHeight,
			m_engineConfig.windowTitle);
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
	GNS_PROFILE_FUNCTION();
	
	while (!close)
	{
		GNS_PROFILE_SCOPE("Frame");
		gns::Time::StartFrameTime();
		float deltaTime = gns::Time::DeltaTime();
		SystemsManager::Run(deltaTime);
		jobs::JobSystem::FlushJobs();
		
		gns::Time::EndFrameTime();
	}
}

void gns::core::Engine::ShutDown()
{
	GNS_PROFILE_FUNCTION();
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
