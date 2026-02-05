#pragma once
#include "API/API.h"
#include "functional"

namespace gns::core {

	struct EngineConfig {
		bool headless;
		bool InitTetsSystem;
	};

	class Engine {
	public:
		GNS_API Engine(EngineConfig engineConfig);
		GNS_API ~Engine() = default;
		GNS_API void Initialize(std::function<void()> callback);
		GNS_API void Run();
		GNS_API void ShutDown();

		bool close;
	private:
		EngineConfig m_engineConfig;
	};
}