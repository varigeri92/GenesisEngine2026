#pragma once
#include "API/API.h"
#include "functional"
#include <cstdint>
#include <filesystem>
#include <string>

struct SDL_Window;

namespace  gns::rendering
{
	class Renderer;
}
namespace gns::core {

	struct EngineConfig {
		bool headless = false;
		bool InitTetsSystem = false;
		uint32_t windowWidth = 1920;
		uint32_t windowHeight = 1080;
		std::string windowTitle = "Genesis";
		std::filesystem::path projectRoot;
		std::filesystem::path editorResourcesRoot;
	};

	class Engine {
	public:
		GNS_API Engine(EngineConfig engineConfig);
		GNS_API ~Engine() = default;
		GNS_API void Initialize(std::function<void()> callback);
		GNS_API void Run();
		GNS_API void ShutDown();

		GNS_API SDL_Window* GetWindow();
		GNS_API gns::rendering::Renderer& GetRenderer();
		
		bool close;
	private:
		EngineConfig m_engineConfig;
	};
}
