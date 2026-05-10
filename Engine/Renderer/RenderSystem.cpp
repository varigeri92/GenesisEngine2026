#include "gnspch.h"
#include "RenderSystem.h"

#include "../Assets/AssetManager.h"
#include "../Window/WindowSystem.h"
#include "../Object/Mesh.h"
#include "../Object/Texture.h"
#include "../Utils/Path.h"
#include "../Core/ComponentLibrary.h"
#include "../Object/Material.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneManager.h"
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
	const bool hasDrawData = !m_drawData.empty();
	if (hasDrawData)
	{
		BuildSceneDataDescriptor();
	}
	else
	{
		m_hasSceneDataDescriptor = false;
	}

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

const CameraBackend& gns::RenderSystem::GetCamera() const
{
	return m_renderer.m_cameraBackend;
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
		mesh.FreeCPUSide();
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
		if (albedoTexture == nullptr)
		{
			albedoTexture = assets::AssetManager::EnsureTextureLoaded(material.albedo_texture.m_handle);
		}

		if (albedoTexture != nullptr)
		{
			if (!ApplyTexture(*albedoTexture).IsValid())
			{
				return {};
			}
		}
		else
		{
			return {};
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

bool gns::RenderSystem::EnsureDefaultMeshResources()
{
	Shader* shader = m_defaultMeshShader.IsValid() ? Object::Get<Shader>(m_defaultMeshShader) : nullptr;
	if (shader == nullptr)
	{
		std::string fragmentShaderPath = R"(Shaders\default.frag)";
		std::string vertexShaderPath = R"(Shaders\mesh.vert)";
		shader = Object::Create<Shader>(vertexShaderPath, fragmentShaderPath, "default_mesh_shader");
		if (shader == nullptr)
		{
			LOG_ERROR("[RenderSystem]: Failed to create default mesh shader.");
			return false;
		}

		ApplyShader(*shader);
		m_defaultMeshShader = shader->GetHandle();
	}

	Material* material = m_defaultMeshMaterial.IsValid() ? Object::Get<Material>(m_defaultMeshMaterial) : nullptr;
	if (material == nullptr)
	{
		material = Object::Create<Material>();
		if (material == nullptr)
		{
			LOG_ERROR("[RenderSystem]: Failed to create default mesh material.");
			return false;
		}

		material->shader_ref = shader->Ref<Shader>();
		material->albedo_color = glm::vec4(0.5f, 1.0f, 0.0f, 1.0f);
		material->SetVec4("albedo_color", material->albedo_color);
		material->albedo_texture = Reference<Texture>(GetDefaultTextureHandle(DefaultTexture::ErrorCheckerboard));
		ApplyMaterial(*material);
		m_defaultMeshMaterial = material->GetHandle();
	}

	return true;
}

gns::Handle gns::RenderSystem::GetDefaultMeshShaderHandle() const
{
	return m_defaultMeshShader;
}

gns::Handle gns::RenderSystem::GetDefaultMeshMaterialHandle() const
{
	return m_defaultMeshMaterial;
}

void gns::RenderSystem::BuildDrawData()
{
	const size_t previousDrawCount = m_drawData.size();
	m_drawData.clear();
	m_drawData.reserve(previousDrawCount);
	if (!EnsureDefaultMeshResources())
	{
		return;
	}

	Shader* defaultShader = Object::Get<Shader>(m_defaultMeshShader);
	if (defaultShader == nullptr)
	{
		LOG_ERROR("[RenderSystem]: Cannot build draw data because default mesh shader is missing.");
		return;
	}

	core::SystemsManager::ForEach<SceneMemberComponent, Transform, MeshComponent>([&](
		SceneMemberComponent& sceneMember,
		Transform& transform,
		MeshComponent& meshComp)
	{
		if (!SceneManager::IsSceneLoaded(sceneMember.scene_handle))
		{
			return;
		}

		if (!meshComp.mesh.m_handle.IsValid() ||
			!meshComp.material.m_handle.IsValid())
		{
			return;
		}

		if (!m_resourceCache.meshes.contains(meshComp.mesh.m_handle))
		{
			if (Mesh* mesh = assets::AssetManager::EnsureMeshLoaded(meshComp.mesh.m_handle))
			{
				ApplyMesh(*mesh);
			}
		}

		Material* material = Object::Get<Material>(meshComp.material.m_handle);
		if (material == nullptr)
		{
			material = assets::AssetManager::EnsureMaterialLoaded(meshComp.material.m_handle);
		}

		if (material == nullptr)
		{
			return;
		}

		Shader* shader = nullptr;
		if (material->shader_ref.m_handle.IsValid())
		{
			shader = Object::Get<Shader>(material->shader_ref.m_handle);
		}

		if (shader == nullptr)
		{
			material->shader_ref = defaultShader->Ref<Shader>();
			shader = defaultShader;
		}

		const Handle shaderHandle = shader->GetHandle();
		if (!m_resourceCache.shaders.contains(shaderHandle))
		{
			ApplyShader(*shader);
		}

		if (!m_resourceCache.materials.contains(meshComp.material.m_handle))
		{
			if (!ApplyMaterial(*material).IsValid())
			{
				return;
			}
		}

		const Handle renderShaderHandle = GetRenderShaderHandle(shaderHandle);
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

		const MaterialLayout& shaderMaterialLayout = vulkanShader->GetMaterialLayout();
		if (shaderMaterialLayout.IsValid() &&
			!material->GetLayout().IsCompatibleWith(shaderMaterialLayout))
		{
			material->SetLayout(shaderMaterialLayout, true);
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
		drawData.transform = m_renderer.m_cameraBackend.viewProjection * transform.matrix;
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
	m_sceneData = SceneData();

	bool hasAmbientLight = false;
	core::SystemsManager::ForEach<SceneMemberComponent, AmbientLightComponent>([&](
		SceneMemberComponent& sceneMember,
		AmbientLightComponent& ambientLight)
	{
		if (hasAmbientLight || !SceneManager::IsSceneLoaded(sceneMember.scene_handle))
		{
			return;
		}

		m_sceneData.ambientColor = ambientLight.color;
		hasAmbientLight = true;
	});

	bool hasDirectionalLight = false;
	core::SystemsManager::ForEach<SceneMemberComponent, DirectionalLightComponent>([&](
		SceneMemberComponent& sceneMember,
		DirectionalLightComponent& directionalLight)
	{
		if (hasDirectionalLight || !SceneManager::IsSceneLoaded(sceneMember.scene_handle))
		{
			return;
		}

		m_sceneData.sunlightDirection = directionalLight.direction;
		m_sceneData.sunlightColor = directionalLight.color;
		hasDirectionalLight = true;
	});

	m_sceneDataDescriptor = GpuDataDescriptor::GetFromType<gns::SceneData>(&m_sceneData);
	m_hasSceneDataDescriptor = true;
}
