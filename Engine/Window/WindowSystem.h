#pragma once
#include "../Systems/System.h"
#include <cstdint>
#include <memory>
#include <string>
#include "Window.h"

namespace gns::core {
	class Engine;
}

namespace gns::window {

class WindowSystem : public core::System
{
public:
	WindowSystem(gns::core::Engine* engine, uint32_t width, uint32_t height, std::string title);
	WindowSystem() = default;
	~WindowSystem() = default;

	// Inherited via System
	void OnCreate() override;
	void OnStart() override;
	void OnEnable() override;
	void OnUpdate(float deltaTime) override;
	void OnFixedUpdate() override;
	void OnDisable() override;
	void OnDestroy() override;

	bool ShouldClose();
	GNS_API SDL_Window* GetSDLWindow();
	GNS_API const gns::Screen& GetScreen() const;
	void MinimizeWindow();
	void ToggleMaximizeWindow();
	bool IsMaximized() const;
	void RequestClose();
	void OnLateUpdate(float deltaTime) override;

private:
	gns::core::Engine* m_engine = nullptr;
	std::unique_ptr<Window> m_window;
	uint32_t m_initialWidth = 1920;
	uint32_t m_initialHeight = 1080;
	std::string m_title = "Genesis";
	bool m_manuallyMaximized = false;
	int m_restoreX = 0;
	int m_restoreY = 0;
	int m_restoreWidth = 0;
	int m_restoreHeight = 0;
};
}
