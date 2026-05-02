#include "gnspch.h"
#include "RenderSystem.h"

#include "../Assets/AssetManager.h"
#include "../Window/WindowSystem.h"
#include "../Core/Entity.h"
#include "../Object/Mesh.h"
#include "../Object/Texture.h"
#include "../Utils/Path.h"
#include "../Core/ComponentLibrary.h"
#include "../Object/Material.h"
#include "../Scene/Scene.h"
#include "../Systems/SystemsManager.h"
#include "Resources/VulkanShader.h"
#include "Resources/VulkanTexture.h"
#include "Vulkan/VulkanMesh.h"

gns::RenderSystem::RenderSystem(gns::window::WindowSystem* ws) : m_windowSystem(ws), m_renderer()
{
}

void gns::RenderSystem::OnCreate()
{
	m_renderer.CreateDevice(m_windowSystem->GetSDLWindow());
	CreateDefaultTextureObjects();
	LOG_INFO("Render System created!");
}

void gns::RenderSystem::OnStart()
{
	gns::Entity sceneEntity = gns::Entity::CreateEntity("SceneDataEntity");
	sceneEntity.AddComponent<SceneData>();
	
	std::string fragmentShaderPath = gns::path::InResourcesDirectory(R"(Shaders\default.frag)").string();
	std::string vertexShaderPath = gns::path::InResourcesDirectory(R"(Shaders\mesh.vert)").string();
	Shader* _shader = Object::Create<Shader>(vertexShaderPath, fragmentShaderPath, "default_mesh_shader");
	_shader->Apply();
	Material* material = Object::Create<Material>();
	material->shader_ref = _shader->Ref<Shader>();
	material->albedo_color = glm::vec4(0.5f, 1.0f, 0.0f, 1.0f);
	material->albedo_texture = Reference<Texture>(GetDefaultTextureHandle(DefaultTexture::ErrorCheckerboard));
	ApplyMaterial(*material);
	Reference<Material> materialRef = material->Ref<Material>();
	
	// NOTE: Startup currently loads a local sample asset directly; project/scene bootstrapping is not defined yet.
	std::vector<gns::assets::LoadedObject> loaded 
		= assets::AssetManager::LoadAsset(
			R"(D:\ProjectGenesis\TestFiles\lucilla_-_vampiric_drake\scene.gltf)");
	for (auto& loaded_object : loaded)
	{
		Mesh* mesh = loaded_object.As<Mesh>();
		mesh->Apply();
		Reference<Material> meshMaterial = materialRef;
		if (loaded_object.materialHandle.IsValid())
		{
			Material* loadedMaterial = Object::Get<Material>(loaded_object.materialHandle);
			if (loadedMaterial != nullptr)
			{
				loadedMaterial->shader_ref = _shader->Ref<Shader>();
				ApplyMaterial(*loadedMaterial);
				meshMaterial = loadedMaterial->Ref<Material>();
			}
		}

		std::string name = mesh->GetName();
		gns::Entity entity = gns::Entity::CreateEntity(name);
		MeshComponent& mesh_comp = entity.AddComponent<MeshComponent>();
		mesh_comp.mesh = mesh->Ref<Mesh>();
		mesh_comp.shader = _shader->Ref<Shader>();
		mesh_comp.material = meshMaterial;
		LOG_INFO(name);
	}
}

void gns::RenderSystem::OnEnable()
{
}

void gns::RenderSystem::OnUpdate(float deltaTime)
{
}

void gns::RenderSystem::OnLateUpdate(float deltaTime)
{
	BuildDrawData();
	BuildSceneDataDescriptor();
	m_renderer.DrawFrame(m_drawData, m_hasSceneDataDescriptor ? &m_sceneDataDescriptor : nullptr);
}

void gns::RenderSystem::OnFixedUpdate()
{
}

void gns::RenderSystem::OnDisable()
{
}

void gns::RenderSystem::OnDestroy()
{
}

gns::rendering::Renderer& gns::RenderSystem::GetRenderer()
{
	return m_renderer;
}

void gns::RenderSystem::WaitForIdle()
{
	m_renderer.WaitForIdle();
}

void gns::RenderSystem::SetCamera(const CameraBackend& camera_backend)
{
	m_renderer.m_cameraBackend = camera_backend;
}

gns::Handle gns::RenderSystem::ApplyMesh(Mesh& mesh)
{
	const Handle meshHandle = mesh.GetHandle();
	if (const auto it = m_resourceCache.meshes.find(meshHandle); it != m_resourceCache.meshes.end())
	{
		return it->second;
	}

	const Handle renderMeshHandle = m_renderer.ApplyMesh(mesh);
	if (renderMeshHandle.IsValid())
	{
		m_resourceCache.meshes[meshHandle] = renderMeshHandle;
	}
	return renderMeshHandle;
}

gns::Handle gns::RenderSystem::ApplyShader(Shader& shader)
{
	const Handle shaderHandle = shader.GetHandle();
	if (const auto it = m_resourceCache.shaders.find(shaderHandle); it != m_resourceCache.shaders.end())
	{
		return it->second;
	}

	const Handle renderShaderHandle = m_renderer.CreateVulkanShader(shader);
	if (renderShaderHandle.IsValid())
	{
		m_resourceCache.shaders[shaderHandle] = renderShaderHandle;
	}
	return renderShaderHandle;
}

gns::Handle gns::RenderSystem::ApplyTexture(Texture& texture)
{
	const Handle textureHandle = texture.GetHandle();
	if (const auto it = m_resourceCache.textures.find(textureHandle); it != m_resourceCache.textures.end())
	{
		return it->second;
	}

	const Handle renderTextureHandle = m_renderer.ApplyTexture(texture);
	if (renderTextureHandle.IsValid())
	{
		m_resourceCache.textures[textureHandle] = renderTextureHandle;
		texture.FreeCPUSide();
	}
	return renderTextureHandle;
}

gns::Handle gns::RenderSystem::ApplyMaterial(Material& material)
{
	const Handle materialHandle = material.GetHandle();
	if (material.albedo_texture.m_handle.IsValid() &&
		!m_resourceCache.textures.contains(material.albedo_texture.m_handle))
	{
		Texture* albedoTexture = Object::Get<Texture>(material.albedo_texture.m_handle);
		if (albedoTexture != nullptr)
		{
			ApplyTexture(*albedoTexture);
		}
	}

	if (const auto it = m_resourceCache.materials.find(materialHandle); it != m_resourceCache.materials.end())
	{
		return it->second;
	}

	m_resourceCache.materials[materialHandle] = materialHandle;
	return materialHandle;
}

gns::Handle gns::RenderSystem::GetRenderMeshHandle(Handle meshHandle) const
{
	if (const auto it = m_resourceCache.meshes.find(meshHandle); it != m_resourceCache.meshes.end())
	{
		return it->second;
	}

	LOG_WARNING("[RenderSystem]: Missing Vulkan mesh resource for engine mesh handle.");
	LOG_WARNING(std::to_string(meshHandle.Get()));
	return {};
}

gns::Handle gns::RenderSystem::GetRenderShaderHandle(Handle shaderHandle) const
{
	if (const auto it = m_resourceCache.shaders.find(shaderHandle); it != m_resourceCache.shaders.end())
	{
		return it->second;
	}

	LOG_WARNING("[RenderSystem]: Missing Vulkan shader resource for engine shader handle.");
	LOG_WARNING(std::to_string(shaderHandle.Get()));
	return {};
}

gns::Handle gns::RenderSystem::GetRenderMaterialHandle(Handle materialHandle) const
{
	if (const auto it = m_resourceCache.materials.find(materialHandle); it != m_resourceCache.materials.end())
	{
		return it->second;
	}

	LOG_WARNING("[RenderSystem]: Missing render material resource for engine material handle.");
	LOG_WARNING(std::to_string(materialHandle.Get()));
	return {};
}

gns::Handle gns::RenderSystem::GetDefaultTextureHandle(DefaultTexture texture) const
{
	switch (texture)
	{
	case DefaultTexture::White:
		return m_defaultTextures.white;
	case DefaultTexture::Grey:
		return m_defaultTextures.grey;
	case DefaultTexture::Black:
		return m_defaultTextures.black;
	case DefaultTexture::ErrorCheckerboard:
		return m_defaultTextures.errorCheckerboard;
	default:
		LOG_WARNING("[RenderSystem]: Unknown default texture requested.");
		return {};
	}
}

gns::RenderTextureBinding gns::RenderSystem::GetTextureBinding(Handle textureHandle)
{
	if (!textureHandle.IsValid())
	{
		LOG_WARNING("[RenderSystem]: Cannot get texture binding for invalid texture handle.");
		return {};
	}

	const auto textureResource = m_resourceCache.textures.find(textureHandle);
	if (textureResource == m_resourceCache.textures.end())
	{
		LOG_WARNING("[RenderSystem]: Missing Vulkan texture resource for engine texture handle.");
		LOG_WARNING(std::to_string(textureHandle.Get()));
		return {};
	}

	rendering::VulkanTexture* texture = m_renderer.GetVulkanTexture(textureResource->second);
	if (texture == nullptr)
	{
		LOG_WARNING("[RenderSystem]: Missing Vulkan texture resource for texture handle.");
		LOG_WARNING(std::to_string(textureResource->second.Get()));
		return {};
	}

	if (texture->descriptorSet == VK_NULL_HANDLE)
	{
		LOG_WARNING("[RenderSystem]: Vulkan texture has no descriptor set.");
		LOG_WARNING(std::to_string(textureResource->second.Get()));
		return {};
	}

	return RenderTextureBinding
	{
		.descriptor = (uint64_t)texture->descriptorSet
	};
}

uint64_t gns::RenderSystem::GetTextureDescriptor(Handle textureHandle)
{
	return GetTextureBinding(textureHandle).descriptor;
}

uint64_t gns::RenderSystem::GetSceneTextureDescriptor()
{
	return m_renderer.GetSceneTextureDescriptor();
}

void gns::RenderSystem::SetScreen(const Screen& screen)
{
	m_renderer.SetScreen(screen);
}

const gns::Screen& gns::RenderSystem::GetScreen() const
{
	return m_renderer.GetScreen();
}

void gns::RenderSystem::CreateDefaultTextureObjects()
{
	const rendering::VulkanDefaultTextureHandles& vulkanDefaults = m_renderer.GetDefaultTextures();

	m_defaultTextures.white = RegisterDefaultTexture(DefaultResourceNames::WhiteTexture, vulkanDefaults.white);
	m_defaultTextures.grey = RegisterDefaultTexture(DefaultResourceNames::GreyTexture, vulkanDefaults.grey);
	m_defaultTextures.black = RegisterDefaultTexture(DefaultResourceNames::BlackTexture, vulkanDefaults.black);
	m_defaultTextures.errorCheckerboard = RegisterDefaultTexture(
		DefaultResourceNames::ErrorCheckerboardTexture,
		vulkanDefaults.errorCheckerboard);
}

gns::Handle gns::RenderSystem::RegisterDefaultTexture(const char* name, Handle vulkanTextureHandle)
{
	if (!vulkanTextureHandle.IsValid())
	{
		LOG_ERROR("[RenderSystem]: Cannot register default texture because Vulkan texture handle is invalid.");
		LOG_ERROR(name);
		return {};
	}

	Texture* texture = Object::Create<Texture>(name);
	if (texture == nullptr)
	{
		LOG_ERROR("[RenderSystem]: Cannot create engine default texture object.");
		LOG_ERROR(name);
		return {};
	}

	const Handle textureHandle = texture->GetHandle();
	m_resourceCache.textures[textureHandle] = vulkanTextureHandle;
	return textureHandle;
}

void gns::RenderSystem::BuildDrawData()
{
	const size_t previousDrawCount = m_drawData.size();
	m_drawData.clear();
	m_drawData.reserve(previousDrawCount);

	auto view = core::SystemsManager::GetRegistry().view<EntityComponent, Transform, MeshComponent>();
	view.each([&](
		EntityComponent& entityComp,
		Transform& transform,
		MeshComponent& meshComp)
	{
		if (!meshComp.mesh.m_handle.IsValid() ||
			!meshComp.material.m_handle.IsValid() ||
			!meshComp.shader.m_handle.IsValid())
		{
			return;
		}

		const Handle renderShaderHandle = GetRenderShaderHandle(meshComp.shader.m_handle);
		const Handle renderMeshHandle = GetRenderMeshHandle(meshComp.mesh.m_handle);
		if (!renderShaderHandle.IsValid() || !renderMeshHandle.IsValid())
		{
			return;
		}

		rendering::VulkanShader* vulkanShader = m_renderer.GetVulkanShader(renderShaderHandle);
		VulkanMesh* vulkanMesh = m_renderer.GetVulkanMesh(renderMeshHandle);
		if (vulkanShader == nullptr || vulkanMesh == nullptr)
		{
			return;
		}

		Material* material = Object::Get<Material>(meshComp.material.m_handle);
		if (material == nullptr)
		{
			return;
		}

		Handle albedoTextureHandle = material->albedo_texture.m_handle;
		if (!albedoTextureHandle.IsValid())
		{
			albedoTextureHandle = GetDefaultTextureHandle(DefaultTexture::White);
		}

		const RenderTextureBinding albedoTextureBinding = GetTextureBinding(albedoTextureHandle);
		if (!albedoTextureBinding.IsValid())
		{
			return;
		}

		DrawData drawData;
		// NOTE: Draw transform currently uses camera view-projection directly; entity/world transform composition is not explicit here yet.
		drawData.transform = m_renderer.m_cameraBackend.viewProjection;
		drawData.vkShader = vulkanShader;
		drawData.vk_indexBuffer = vulkanMesh->indexBuffer.buffer;
		drawData.albedoTextureDescriptor = albedoTextureBinding.descriptor;
		drawData.vk_vertexBufferAddress = vulkanMesh->vertexBufferAddress;
		drawData.StartIndex = vulkanMesh->startIndex;
		drawData.Count = vulkanMesh->count;
		m_drawData.emplace_back(drawData);
	});
}

void gns::RenderSystem::BuildSceneDataDescriptor()
{
	m_hasSceneDataDescriptor = false;
	auto sceneDataView = core::SystemsManager::GetRegistry()
		.view<EntityComponent, Transform, gns::SceneData>();
	sceneDataView.each([&](
		EntityComponent& entityComp,
		Transform& transform,
		gns::SceneData& sceneData)
	{
		m_sceneDataDescriptor = GpuDataDescriptor::GetFromType<gns::SceneData>(&sceneData);
		m_hasSceneDataDescriptor = true;
	});
}
