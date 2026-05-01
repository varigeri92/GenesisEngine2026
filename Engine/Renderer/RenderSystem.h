#pragma once
#include <cstdint>
#include <unordered_map>

#include "../Core/Handles.h"
#include "../Systems/System.h"
#include "Renderer.h"

struct CameraBackend;

namespace gns::window {
	class WindowSystem;
}
namespace gns {
	struct Mesh;
	struct Shader;
	struct Texture;

	enum class DefaultTexture
	{
		White,
		Grey,
		Black,
		ErrorCheckerboard
	};

	struct RenderTextureBinding
	{
		uint64_t descriptor = 0;

		bool IsValid() const
		{
			return descriptor != 0;
		}
	};

	struct RenderResourceCache
	{
		std::unordered_map<Handle, Handle> meshes;
		std::unordered_map<Handle, Handle> shaders;
		std::unordered_map<Handle, Handle> textures;
		std::unordered_map<Handle, Handle> materials;
	};

	struct EngineDefaultTextureHandles
	{
		Handle white;
		Handle grey;
		Handle black;
		Handle errorCheckerboard;
	};

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
		GNS_API Handle ApplyMesh(Mesh& mesh);
		GNS_API Handle CreateVulkanShader(Shader& shader);
		GNS_API Handle GetVulkanMeshHandle(Handle meshHandle) const;
		GNS_API Handle GetVulkanShaderHandle(Handle shaderHandle) const;
		GNS_API Handle GetDefaultTextureHandle(DefaultTexture texture) const;
		GNS_API RenderTextureBinding GetTextureBinding(Handle textureHandle);
		GNS_API uint64_t GetTextureDescriptor(Handle textureHandle);

	private:
		gns::window::WindowSystem* m_windowSystem;
		rendering::Renderer m_renderer;
		RenderResourceCache m_resourceCache;
		EngineDefaultTextureHandles m_defaultTextures;

		void CreateDefaultTextureObjects();
		Handle RegisterDefaultTexture(const char* name, Handle vulkanTextureHandle);
	};
}
