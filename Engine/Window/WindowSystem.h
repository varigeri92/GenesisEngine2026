#pragma once
#include "../Systems/System.h"
#include <memory>
#include "Window.h"

namespace gns::core {
	class Engine;
}

namespace gns::window {

class WindowSystem : public core::System
{
public:
	WindowSystem(gns::core::Engine* engine);
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
	void OnLateUpdate(float deltaTime) override;

private:
	gns::core::Engine* m_engine;
	std::unique_ptr<Window> m_window;
};
}
