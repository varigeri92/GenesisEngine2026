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
#include "Resources/VulkanMaterial.h"
#include "Resources/VulkanShader.h"
#include "Resources/VulkanTexture.h"
#include "Vulkan/VulkanMesh.h"

namespace
{
	constexpr uint32_t MaterialDataSet = 1;
	constexpr uint32_t MaterialDataBinding = 0;
	constexpr uint32_t MaterialTextureSet = 2;

	bool TryFindMaterialDataBinding(
		const gns::Material& material,
		gns::MaterialPropertyInfo& outProperty)
	{
		bool found = false;
		for (const gns::MaterialPropertyInfo& property : material.GetProperties())
		{
			if (!property.IsBufferBacked() ||
				property.descriptorKind == gns::MaterialDescriptorKind::None)
			{
				continue;
			}

			if (property.set != MaterialDataSet ||
				property.binding != MaterialDataBinding ||
				(property.descriptorKind != gns::MaterialDescriptorKind::UniformBuffer &&
					property.descriptorKind != gns::MaterialDescriptorKind::StorageBuffer))
			{
				LOG_ERROR("[RenderSystem]: Material buffer property violates the material descriptor rules.");
				LOG_ERROR(property.name);
				return false;
			}

			if (!found)
			{
				outProperty = property;
				found = true;
				continue;
			}

			if (outProperty.descriptorKind != property.descriptorKind)
			{
				LOG_ERROR("[RenderSystem]: Material buffer properties use conflicting descriptor kinds.");
				LOG_ERROR(property.name);
				return false;
			}
		}

		return found;
	}
}

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
	if (material.shader_ref.m_handle.IsValid())
	{
		Shader* shader = Object::Get<Shader>(material.shader_ref.m_handle);
		if (shader != nullptr)
		{
			const Handle shaderHandle = shader->GetHandle();
			if (!m_resourceCache.shaders.contains(shaderHandle))
			{
				ApplyShader(*shader);
			}

			const Handle renderShaderHandle = GetRenderShaderHandle(shaderHandle);
			rendering::VulkanShader* vulkanShader = renderShaderHandle.IsValid()
				? m_renderer.GetVulkanShader(renderShaderHandle)
				: nullptr;
			if (vulkanShader != nullptr)
			{
				const MaterialLayout& shaderMaterialLayout = vulkanShader->GetMaterialLayout();
				if (!material.GetLayout().IsCompatibleWith(shaderMaterialLayout))
				{
					material.SetLayout(shaderMaterialLayout, true);
					material.ApplyImportCompatibilityDefaults();
				}
				return ApplyMaterial(material, *vulkanShader);
			}
		}
	}

	auto ensureTextureApplied = [&](Handle textureHandle) -> bool
	{
		if (!textureHandle.IsValid() || m_resourceCache.textures.contains(textureHandle))
		{
			return true;
		}

		Texture* texture = Object::Get<Texture>(textureHandle);
		if (texture == nullptr)
		{
			texture = assets::AssetManager::EnsureTextureLoaded(textureHandle);
		}

		if (texture == nullptr)
		{
			return false;
		}

		return ApplyTexture(*texture).IsValid();
	};

	for (size_t textureIndex = 0; textureIndex < material.GetTextureSlotCount(); ++textureIndex)
	{
		const Handle textureHandle = material.GetTextureHandle(textureIndex);
		if (!ensureTextureApplied(textureHandle))
		{
			return {};
		}
	}

	if (const auto it = m_resourceCache.materials.find(materialHandle); it != m_resourceCache.materials.end())
	{
		return it->second;
	}

	return {};
}

gns::Handle gns::RenderSystem::ApplyMaterial(Material& material, rendering::VulkanShader& vulkanShader)
{
	const Handle materialHandle = material.GetHandle();

	auto ensureTextureApplied = [&](Handle textureHandle) -> bool
	{
		if (!textureHandle.IsValid() || m_resourceCache.textures.contains(textureHandle))
		{
			return true;
		}

		Texture* texture = Object::Get<Texture>(textureHandle);
		if (texture == nullptr)
		{
			texture = assets::AssetManager::EnsureTextureLoaded(textureHandle);
		}

		if (texture == nullptr)
		{
			return false;
		}

		return ApplyTexture(*texture).IsValid();
	};

	std::vector<MaterialTextureBinding> textureBindings;
	textureBindings.reserve(material.GetTextureSlotCount());
	for (size_t textureIndex = 0; textureIndex < material.GetTextureSlotCount(); ++textureIndex)
	{
		const MaterialPropertyInfo* property = material.GetTextureSlotProperty(textureIndex);
		if (property == nullptr)
		{
			LOG_ERROR("[RenderSystem]: Material texture slot is missing metadata.");
			return {};
		}

		if (property->set != MaterialTextureSet || property->descriptorCount != 1)
		{
			LOG_ERROR("[RenderSystem]: Material texture property violates the material texture binding rules.");
			LOG_ERROR(property->name);
			return {};
		}

		Handle textureHandle = material.GetTextureHandle(textureIndex);
		if (!textureHandle.IsValid())
		{
			textureHandle = GetDefaultTextureHandle(DefaultTexture::White);
		}

		if (!ensureTextureApplied(textureHandle))
		{
			return {};
		}

		const auto textureResource = m_resourceCache.textures.find(textureHandle);
		if (textureResource == m_resourceCache.textures.end())
		{
			LOG_ERROR("[RenderSystem]: Missing Vulkan texture resource for material texture.");
			LOG_ERROR(std::to_string(textureHandle.Get()));
			return {};
		}

		rendering::VulkanTexture* vulkanTexture = m_renderer.GetVulkanTexture(textureResource->second);
		if (vulkanTexture == nullptr)
		{
			return {};
		}

		textureBindings.emplace_back(MaterialTextureBinding
		{
			.binding = property->binding,
			.texture = vulkanTexture
		});
	}

	GpuDataDescriptor materialDataDescriptor;
	uint32_t materialDataSet = gns::InvalidMaterialBinding;
	uint32_t materialDataBinding = gns::InvalidMaterialBinding;
	MaterialDescriptorKind materialDataDescriptorKind = MaterialDescriptorKind::None;
	const MaterialDataBlob materialDataBlob = material.GetDataBlob();
	if (materialDataBlob.IsValid())
	{
		MaterialPropertyInfo materialDataProperty;
		if (TryFindMaterialDataBinding(material, materialDataProperty))
		{
			materialDataDescriptor = GpuDataDescriptor::GetFromMemory(
				materialDataBlob.data,
				materialDataBlob.size);
			materialDataSet = materialDataProperty.set;
			materialDataBinding = materialDataProperty.binding;
			materialDataDescriptorKind = materialDataProperty.descriptorKind;
		}
	}

	rendering::VulkanMaterial* vulkanMaterial = nullptr;
	Handle renderMaterialHandle;
	if (const auto it = m_resourceCache.materials.find(materialHandle); it != m_resourceCache.materials.end())
	{
		renderMaterialHandle = it->second;
		vulkanMaterial = m_renderer.GetVulkanMaterial(renderMaterialHandle);
	}

	if (vulkanMaterial == nullptr)
	{
		vulkanMaterial = m_renderer.m_device.CreateResource<rendering::VulkanMaterial>();
		if (vulkanMaterial == nullptr)
		{
			LOG_ERROR("[RenderSystem]: Failed to create Vulkan material resource.");
			return {};
		}

		renderMaterialHandle = vulkanMaterial->GetHandle();
		m_resourceCache.materials[materialHandle] = renderMaterialHandle;
	}

	if (!m_renderer.m_device.UpdateMaterialResource(
		*vulkanMaterial,
		vulkanShader,
		materialDataDescriptor,
		materialDataSet,
		materialDataBinding,
		materialDataDescriptorKind,
		textureBindings,
		MaterialTextureSet))
	{
		return {};
	}

	return renderMaterialHandle;
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
		material->albedo_texture = Reference<Texture>(GetDefaultTextureHandle(DefaultTexture::ErrorCheckerboard));
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
		const bool materialLayoutChanged = !material->GetLayout().IsCompatibleWith(shaderMaterialLayout);
		if (materialLayoutChanged)
		{
			material->SetLayout(shaderMaterialLayout, true);
			material->ApplyImportCompatibilityDefaults();
		}

		const Handle renderMaterialHandle = ApplyMaterial(*material, *vulkanShader);
		if (!renderMaterialHandle.IsValid())
		{
			return;
		}

		rendering::VulkanMaterial* vulkanMaterial = m_renderer.GetVulkanMaterial(renderMaterialHandle);
		if (vulkanMaterial == nullptr)
		{
			return;
		}

		DrawData drawData;
		drawData.transform = m_renderer.m_cameraBackend.viewProjection * transform.matrix;
		drawData.vkShader = vulkanShader;
		drawData.vk_indexBuffer = vulkanMesh->indexBuffer.buffer;
		drawData.vkMaterial = vulkanMaterial;
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
