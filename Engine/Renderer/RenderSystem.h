#pragma once
#include "../Systems/System.h"
#include "Renderer.h"

namespace gns::window {
	class WindowSystem;
}
namespace gns {

	class RenderSystem : public gns::core::System
	{

	public:

		RenderSystem() = delete;
		RenderSystem(gns::window::WindowSystem* ws);

		void OnCreate() override;
		void OnStart() override;
		void OnEnable() override;
		void OnUpdate(float deltaTime) override;
		void OnFixedUpdate() override;
		void OnDisable() override;
		void OnDestroy() override;

	private:
		gns::window::WindowSystem* m_windowSystem;
		rendering::Renderer m_renderer;
	};
}