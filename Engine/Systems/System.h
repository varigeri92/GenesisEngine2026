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
	GNS_API virtual void OnCreate(){}
	GNS_API virtual void OnStart(){}
	GNS_API virtual void OnEnable(){}
	GNS_API virtual void OnUpdate(float deltaTime){}
	GNS_API virtual void OnLateUpdate(float deltaTime){}
	GNS_API virtual void OnFixedUpdate(){}
	GNS_API virtual void OnDisable(){}
	GNS_API virtual void OnDestroy(){}

	SystemState State = SystemState::Created;
protected:
	size_t m_instanceID;
	bool m_allowMultipleInstances = false;
};
}