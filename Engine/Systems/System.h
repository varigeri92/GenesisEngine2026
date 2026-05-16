#pragma once
#include "API.h"
#include <chrono>

struct TimerData
{
	double timeMs = 0.0;
	double smoothedTimeMs = 0.0;
	bool hasTimingSample = false;
};

struct SytemMetadata
{
	std::string name;
	TimerData _createTimer;
	TimerData _startupTimer;
	TimerData _enableTimer;
	TimerData _disableTimer;
	TimerData _updateTimer;
	TimerData _lateTimer;
	
	
};

struct SystemScopeTimer
{
	std::chrono::steady_clock::time_point startTime;
	TimerData& timerData;
	GNS_API SystemScopeTimer(TimerData& timer_data);
	GNS_API ~SystemScopeTimer();
};

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
	SytemMetadata metadata;
	
protected:
	size_t m_instanceID;
	bool m_allowMultipleInstances = false;
};
}