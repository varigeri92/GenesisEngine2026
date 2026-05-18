#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "../Core/Handles.h"
#include "../Scene/Scene.h"
#include "../Systems/System.h"
#include "Renderer.h"
#include "RenderThread.h"

struct CameraBackend;

namespace gns::window {
	class WindowSystem;
}
namespace gns {
	struct Material;
	struct Mesh;
	struct Shader;
	struct Texture;
	enum class DefaultTexture
	{
		White,
		Grey,
		Black,
		Normal,
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
		Handle normal;
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
		GNS_API const CameraBackend& GetCamera() const;
		GNS_API Handle ApplyMesh(Mesh& mesh);
		GNS_API Handle ApplyShader(Shader& shader);
		GNS_API Handle ApplyTexture(Texture& texture);
		GNS_API Handle ApplyMaterial(Material& material);
		GNS_API Handle GetRenderMeshHandle(Handle meshHandle) const;
		GNS_API Handle GetRenderShaderHandle(Handle shaderHandle) const;
		GNS_API Handle GetRenderMaterialHandle(Handle materialHandle) const;
		GNS_API Handle GetDefaultTextureHandle(DefaultTexture texture) const;
		GNS_API RenderTextureBinding GetTextureBinding(Handle textureHandle);
		GNS_API uint64_t GetTextureDescriptor(Handle textureHandle);
		GNS_API uint64_t GetSceneTextureDescriptor();
		GNS_API void SetScreen(const Screen& screen);
		GNS_API bool EnsureDefaultMeshResources();
		GNS_API Handle GetDefaultMeshShaderHandle() const;
		GNS_API Handle GetDefaultMeshMaterialHandle() const;

	private:
		gns::window::WindowSystem* m_windowSystem;
		rendering::Renderer m_renderer;
		RenderThread m_renderThread;
		RenderResourceCache m_resourceCache;
		EngineDefaultTextureHandles m_defaultTextures;
		RenderFramePacket m_framePacket;
		RenderUploadQueue m_pendingUploads;
		std::vector<glm::mat4> m_modelMatrices;
		std::vector<rendering::VulkanMaterial*> m_materials;
		SceneData m_sceneData = {};
		DirectionalLightBuffer m_directionalLights = {};
		PointLightBuffer m_pointLights = {};
		SpotLightBuffer m_spotLights = {};
		GlobalFrameDataDescriptor m_globalFrameDataDescriptor = {};
		bool m_hasGlobalFrameDataDescriptor = false;
		Handle m_defaultMeshShader;
		Handle m_defaultMeshMaterial;

		void CreateDefaultTextureObjects();
		Handle RegisterDefaultTexture(const char* name, Handle vulkanTextureHandle);
		Handle ApplyMaterial(Material& material, rendering::VulkanShader& vulkanShader, RenderUploadQueue* dependencyUploads = nullptr);
		bool QueueMeshUpload(Mesh& mesh);
		bool QueueTextureUpload(Texture& texture);
		bool QueueTextureUpload(RenderUploadQueue& uploads, Texture& texture);
		bool QueueShaderUpload(Shader& shader);
		bool QueueShaderUpload(RenderUploadQueue& uploads, Shader& shader);
		bool QueueMaterialUpload(Material& material);
		RenderUploadQueue ConsumePendingRenderUploads();
		void HarvestCompletedRenderSubmissions();
		void RequeuePendingRenderUploads(RenderUploadQueue& uploads);
		void FlushRenderUploads(RenderUploadQueue& uploads);
		void ExecuteRenderSubmission(RenderSubmission& submission);
		void BuildDrawData();
		void BuildGlobalFrameDataDescriptor();
	};
}
