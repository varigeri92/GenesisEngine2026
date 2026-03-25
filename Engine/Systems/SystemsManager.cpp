#include "gnspch.h"
#include "SystemsManager.h"
#include "SystemsManager.h"

std::vector<std::unique_ptr<gns::core::System>> gns::core::SystemsManager::Systems = {};
std::vector<size_t> gns::core::SystemsManager::deletionQeue = {};

void gns::core::SystemsManager::Run(float deltaTime)
{
	bool firstUpdateTicked = false;
	for (size_t i = 0; i < Systems.size(); i++)
	{
		switch (Systems[i]->State)
		{
		case System::SystemState::Created:
			Systems[i]->State = System::SystemState::Started;
			Systems[i]->OnCreate();
			break; 
		case System::SystemState::Started:
			Systems[i]->OnStart();
			Systems[i]->State = System::SystemState::Running;
			break;
		case System::SystemState::Stopped:
			Systems[i]->OnDisable();
			Systems[i]->State = System::SystemState::Disabled;
			break;
		case System::SystemState::Disabled:
			continue;
			break;
		case System::SystemState::Enabled:
			Systems[i]->OnEnable();
			Systems[i]->State = System::SystemState::Running;
			break;
		case System::SystemState::Running:
			Systems[i]->OnUpdate(deltaTime);
			firstUpdateTicked = true;
			break;
		case System::SystemState::Destroyed:
			Systems[i]->OnDestroy();
			break;
		default:
			continue;
			break;
		}
	}
	if (!firstUpdateTicked) return;
	
	for (size_t i = 0; i < Systems.size(); i++)
	{
		if (Systems[i]->State==System::SystemState::Running)
		{
			Systems[i]->OnLateUpdate(deltaTime);
		}
	}
}

void gns::core::SystemsManager::Clear()
{
	for (size_t i = 0; i < Systems.size(); i++)
	{
		Systems[i]->OnDestroy();
	}

	Systems.clear();
}
