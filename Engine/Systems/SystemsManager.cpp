#include "gnspch.h"
#include "SystemsManager.h"

entt::registry gns::core::SystemsManager::Registry = entt::registry();

std::vector<std::unique_ptr<gns::core::System>> gns::core::SystemsManager::Systems = {};
std::vector<size_t> gns::core::SystemsManager::deletionQeue = {};

void gns::core::SystemsManager::Run(float deltaTime)
{
	GNS_PROFILE_FUNCTION();

	bool firstUpdateTicked = false;
	for (size_t i = 0; i < Systems.size(); i++)
	{
#ifdef ENABLE_PROFILER
		const std::string systemScopeName = std::string("System::") + typeid(*Systems[i]).name();
		GNS_PROFILE_SCOPE(systemScopeName.c_str());
#endif

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
#ifdef ENABLE_PROFILER
			const std::string systemScopeName = std::string("System::LateUpdate::") + typeid(*Systems[i]).name();
			GNS_PROFILE_SCOPE(systemScopeName.c_str());
#endif
			Systems[i]->OnLateUpdate(deltaTime);
		}
	}
}

void gns::core::SystemsManager::Clear()
{
	GNS_PROFILE_FUNCTION();

	for (size_t i = 0; i < Systems.size(); i++)
	{
#ifdef ENABLE_PROFILER
		const std::string systemScopeName = std::string("System::Destroy::") + typeid(*Systems[i]).name();
		GNS_PROFILE_SCOPE(systemScopeName.c_str());
#endif
		Systems[i]->OnDestroy();
	}

	Systems.clear();
}

bool gns::core::SystemsManager::IsEntityValid(gns::entityHandle entity)
{
	return entity != gns::NullEntity && Registry.valid(entity);
}

entt::registry& gns::core::SystemsManager::GetRegistry()
{
	return SystemsManager::Registry;
}
