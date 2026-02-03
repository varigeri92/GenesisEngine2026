#pragma once
#include "API/API.h"

namespace gns::core {

	class Engine {
	public:
		GNS_API Engine();
		GNS_API ~Engine() = default;
		GNS_API void Initialize();
		GNS_API void Run();
		GNS_API void ShutDown();
};
}