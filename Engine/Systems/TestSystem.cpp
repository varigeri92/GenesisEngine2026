#include "gnspch.h"
#include "TestSystem.h"

#include "SystemsManager.h"
#include "../Core/ComponentLibrary.h"
#include "../Log/Logger.h"
#include "../Input/InputBackend.h"
#include "SDL2/SDL_keycode.h"
#include "../Core/Handles.h"

gns::core::TestSystem::~TestSystem()
{
	LOG_INFO("Test System Destuctor!");
}

void gns::core::TestSystem::OnCreate()
{
	LOG_INFO("Test System Crated!");
}

void gns::core::TestSystem::OnStart()
{
	LOG_INFO("Test System Started!");
}

void gns::core::TestSystem::OnEnable()
{
	LOG_INFO("Test System Enabled!");

}

void gns::core::TestSystem::OnUpdate(float deltaTime)
{
	
	auto view = SystemsManager::GetRegistry().view<EntityComponent, Transform>();

	LOG_INFO("Test System iterate entity view!");
	view.each([deltaTime](auto &entityComp, auto &transform)
	{
		transform.position = transform.position + (glm::vec3(1,1,1) * deltaTime);
		LOG_INFO(entityComp.name);
		LOG_INFO(std::to_string(transform.position.y));
	});
	
	if (gns::core::InputBackend::GetKeyUp(SDLK_w))
	{
		LOG_INFO("W up");
	}
}

void gns::core::TestSystem::OnFixedUpdate()
{

}

void gns::core::TestSystem::OnDisable()
{
	LOG_INFO("Test System Disabled!");

}

void gns::core::TestSystem::OnDestroy()
{
	LOG_INFO("Test System Destroyed!");
}

void gns::core::TestSystem::OnLateUpdate(float deltaTime)
{
}
