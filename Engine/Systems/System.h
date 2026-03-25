#pragma once
#include "API.h"

namespace gns::core 
{
class System
{
public:
	enum class SystemState
	{
		Created, Started, Running, Stopped, Disabled, Destroyed, Enabled
	};
	

	GNS_API System() = default;
	GNS_API virtual ~System() = default;
	
	GNS_API virtual void OnCreate() = 0;
	GNS_API virtual void OnStart() = 0;
	GNS_API virtual void OnEnable() = 0;
	GNS_API virtual void OnUpdate(float deltaTime) = 0;
	GNS_API virtual void OnLateUpdate(float deltaTime) = 0;
	GNS_API virtual void OnFixedUpdate() = 0;
	GNS_API virtual void OnDisable() = 0;
	GNS_API virtual void OnDestroy() = 0;

	SystemState State = SystemState::Created;
protected:
	size_t m_instanceID;
	bool m_allowMultipleInstances = false;
};
}