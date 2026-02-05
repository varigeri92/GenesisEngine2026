#include "gnspch.h"
#include "Engine.h"
#include "Log/Logger.h"
#include "Systems/SystemsManager.h"
#include "Systems/TestSystem.h"
#include "Window/WindowSystem.h"
#include "Utils/Time.h"
#include "Renderer/RenderSystem.h"

gns::core::Engine::Engine(EngineConfig cfg) : close(false), m_engineConfig{cfg.headless, cfg.InitTetsSystem} {}

void gns::core::Engine::Initialize(std::function<void()> callback)
{
	gns::window::WindowSystem* ws = nullptr;
	if (!m_engineConfig.headless) {
		ws = SystemsManager::RegisterSystem<gns::window::WindowSystem>(this);
		SystemsManager::RegisterSystem<gns::RenderSystem>(ws);
	}
	
	if(m_engineConfig.InitTetsSystem)
		SystemsManager::RegisterSystem<TestSystem>();
	
	callback();
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