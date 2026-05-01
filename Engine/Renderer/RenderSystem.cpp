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
#include "Resources/VulkanTexture.h"

gns::RenderSystem::RenderSystem(gns::window::WindowSystem* ws) : m_windowSystem(ws), m_renderer()
{
	m_renderer.SetRenderSystem(this);
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
	_shader->CreateVulkanShader();
	Material* material = Object::Create<Material>();
	material->shader_ref = _shader->Ref<Shader>();
	material->albedo_color = glm::vec4(0.5f, 1.0f, 0.0f, 1.0f);
	Reference<Material> materialRef = material->Ref<Material>();
	
	std::vector<gns::assets::LoadedObject> loaded 
		= assets::AssetManager::LoadAsset(
			R"(D:\ProjectGenesis\TestFiles\source\Sorcerrer_03.fbx)");
	for (auto& loaded_object : loaded)
	{
		Mesh* mesh = loaded_object.As<Mesh>();
		mesh->Apply();
		std::string name = mesh->GetName();
		gns::Entity entity = gns::Entity::CreateEntity(name);
		MeshComponent& mesh_comp = entity.AddComponent<MeshComponent>();
		mesh_comp.mesh = mesh->Ref<Mesh>();
		mesh_comp.shader = _shader->Ref<Shader>();
		mesh_comp.material = material->Ref<Material>();
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
	BuildRenderItems();
	m_renderer.DrawFrame();
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

	const Handle vulkanMeshHandle = m_renderer.ApplyMesh(mesh);
	if (vulkanMeshHandle.IsValid())
	{
		m_resourceCache.meshes[meshHandle] = vulkanMeshHandle;
	}
	return vulkanMeshHandle;
}

gns::Handle gns::RenderSystem::CreateVulkanShader(Shader& shader)
{
	const Handle shaderHandle = shader.GetHandle();
	if (const auto it = m_resourceCache.shaders.find(shaderHandle); it != m_resourceCache.shaders.end())
	{
		return it->second;
	}

	const Handle vulkanShaderHandle = m_renderer.CreateVulkanShader(shader);
	if (vulkanShaderHandle.IsValid())
	{
		m_resourceCache.shaders[shaderHandle] = vulkanShaderHandle;
	}
	return vulkanShaderHandle;
}

gns::Handle gns::RenderSystem::GetVulkanMeshHandle(Handle meshHandle) const
{
	if (const auto it = m_resourceCache.meshes.find(meshHandle); it != m_resourceCache.meshes.end())
	{
		return it->second;
	}

	LOG_WARNING("[RenderSystem]: Missing Vulkan mesh resource for engine mesh handle.");
	LOG_WARNING(std::to_string(meshHandle.Get()));
	return {};
}

gns::Handle gns::RenderSystem::GetVulkanShaderHandle(Handle shaderHandle) const
{
	if (const auto it = m_resourceCache.shaders.find(shaderHandle); it != m_resourceCache.shaders.end())
	{
		return it->second;
	}

	LOG_WARNING("[RenderSystem]: Missing Vulkan shader resource for engine shader handle.");
	LOG_WARNING(std::to_string(shaderHandle.Get()));
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

const std::vector<gns::RenderItem>& gns::RenderSystem::GetRenderItems() const
{
	return m_renderItems;
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

void gns::RenderSystem::BuildRenderItems()
{
	const size_t previousItemCount = m_renderItems.size();
	m_renderItems.clear();
	m_renderItems.reserve(previousItemCount);

	auto view = core::SystemsManager::GetRegistry().view<EntityComponent, Transform, MeshComponent>();
	view.each([&](
		EntityComponent& entityComp,
		Transform& transform,
		MeshComponent& meshComp)
	{
		RenderItem item =
		{
			.mesh = meshComp.mesh.m_handle,
			.material = meshComp.material.m_handle,
			.shader = meshComp.shader.m_handle,
			.worldTransform = transform.matrix
		};

		if (!item.IsValid())
		{
			return;
		}

		m_renderItems.emplace_back(item);
	});
}
