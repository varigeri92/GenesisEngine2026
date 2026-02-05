#pragma once
#include <chrono>

#include "API.h"

namespace gns
{
	class Time
	{
		static std::chrono::time_point<std::chrono::steady_clock> m_startTime;
		static float m_deltaTime;
	public:
		GNS_API static float DeltaTime(){ return m_deltaTime; };
		static void StartFrameTime();
		static int64_t GetNow();
		static void EndFrameTime();
	};
}
