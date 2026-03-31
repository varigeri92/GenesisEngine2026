#pragma once
#include "../Systems/System.h"
#include "Renderer.h"

struct CameraBackend;

namespace gns::window {
	class WindowSystem;
}
namespace gns {
	struct Mesh;

	class RenderSystem : public gns::core::System
	{

	public:

		RenderSystem() = delete;
		RenderSystem(gns::window::WindowSystem* ws);

		void OnCreate() override;
		void OnStart() override;
		void OnEnable() override;
		void OnUpdate(float deltaTime) override;
		void OnLateUpdate(float deltaTime) override;
		void OnFixedUpdate() override;
		void OnDisable() override;
		void OnDestroy() override;
		
		GNS_API rendering::Renderer& GetRenderer();
		void WaitForIdle();
		GNS_API void SetCamera(const CameraBackend& camera_backend);

	private:
		gns::window::WindowSystem* m_windowSystem;
		rendering::Renderer m_renderer;
	};
}