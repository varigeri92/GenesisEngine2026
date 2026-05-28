#include "gnspch.h"
#include "RenderSystem.h"

#include <algorithm>
#include <iterator>

#include <glm/detail/type_quat.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>

#include "glm/glm.hpp"
#include "../Assets/AssetSystem.h"
#include "../Window/WindowSystem.h"
#include "../Object/Mesh.h"
#include "../Object/Texture.h"
#include "../Utils/Path.h"
#include "../Core/ComponentLibrary.h"
#include "../Object/Material.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneManager.h"
#include "../Systems/SystemsManager.h"
#include "../Utils/TransformHelper.h"
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
		GNS_PROFILE_SCOPE("RenderSystem::TryFindMaterialDataBinding");
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

	gns::DefaultTexture GetDefaultTextureForMaterialSlot(const std::string& slotName)
	{
		if (slotName == "normal_map")
		{
			return gns::DefaultTexture::Normal;
		}

		if (slotName == "metallic_map" ||
			slotName == "emissive_map")
		{
			return gns::DefaultTexture::Black;
		}

		return gns::DefaultTexture::White;
	}

	gns::assets::AssetSystem* GetAssetSystem()
	{
		return gns::core::SystemsManager::GetSystem<gns::assets::AssetSystem>();
	}
}

gns::RenderSystem::RenderSystem(gns::window::WindowSystem* ws) : m_windowSystem(ws), m_renderer()
{
}

void gns::RenderSystem::OnCreate()
{
	GNS_PROFILE_FUNCTION();
	m_renderer.CreateDevice(m_windowSystem->GetSDLWindow());
	CreateDefaultTextureObjects();
	m_renderThread.Start(
		m_renderer,
		[this](RenderSubmission& submission)
		{
			ExecuteRenderSubmission(submission);
		});
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
	GNS_PROFILE_FUNCTION();
	HarvestCompletedRenderSubmissions();
}

void gns::RenderSystem::OnLateUpdate(float deltaTime)
{
	GNS_PROFILE_FUNCTION();
	RenderSubmission submission;
	submission.uploads = ConsumePendingRenderUploads();

	{
		GNS_PROFILE_SCOPE("RenderSystem::BuildRenderFramePacket");
		m_framePacket.Clear();
		BuildDrawData();
		const bool hasDrawData = !m_framePacket.drawData.empty();
		if (hasDrawData)
		{
			BuildGlobalFrameDataDescriptor();
			m_framePacket.globalFrameData = m_globalFrameDataDescriptor;
			m_framePacket.hasGlobalFrameData = m_hasGlobalFrameDataDescriptor;
		}
		else
		{
			m_hasGlobalFrameDataDescriptor = false;
			m_framePacket.hasGlobalFrameData = false;
		}
	}

	submission.packet = m_framePacket;
	m_renderThread.Submit(std::move(submission));
}

void gns::RenderSystem::OnFixedUpdate()
{
}

void gns::RenderSystem::OnDisable()
{
}

void gns::RenderSystem::OnDestroy()
{
	GNS_PROFILE_FUNCTION();
	m_renderThread.Stop();
	HarvestCompletedRenderSubmissions();
	m_renderer.WaitForIdle();
}

gns::rendering::Renderer& gns::RenderSystem::GetRenderer()
{
	return m_renderer;
}

void gns::RenderSystem::WaitForIdle()
{
	GNS_PROFILE_FUNCTION();
	HarvestCompletedRenderSubmissions();
	m_renderer.WaitForIdle();
}

void gns::RenderSystem::SetCamera(const CameraBackend& camera_backend)
{
	GNS_PROFILE_FUNCTION();
	m_renderer.m_cameraBackend = camera_backend;
}

const CameraBackend& gns::RenderSystem::GetCamera() const
{
	return m_renderer.m_cameraBackend;
}

gns::Handle gns::RenderSystem::ApplyMesh(Mesh& mesh)
{
	GNS_PROFILE_FUNCTION();
	const Handle meshHandle = mesh.GetHandle();
	if (const auto it = m_resourceCache.meshes.find(meshHandle); it != m_resourceCache.meshes.end())
	{
		return it->second;
	}

	QueueMeshUpload(mesh);
	return {};
}

bool gns::RenderSystem::QueueMeshUpload(Mesh& mesh)
{
	GNS_PROFILE_FUNCTION();
	const Handle meshHandle = mesh.GetHandle();
	if (!meshHandle.IsValid() || m_resourceCache.meshes.contains(meshHandle))
	{
		return false;
	}

	const auto pending = std::find_if(
		m_pendingUploads.meshUploads.begin(),
		m_pendingUploads.meshUploads.end(),
		[meshHandle](const PendingMeshUpload& upload)
		{
			return upload.meshHandle == meshHandle;
		});
	if (pending != m_pendingUploads.meshUploads.end())
	{
		return true;
	}

	m_pendingUploads.meshUploads.emplace_back(PendingMeshUpload
	{
		.meshHandle = meshHandle,
		.mesh = &mesh
	});
	return true;
}

bool gns::RenderSystem::QueueTextureUpload(Texture& texture)
{
	GNS_PROFILE_FUNCTION();
	return QueueTextureUpload(m_pendingUploads, texture);
}

bool gns::RenderSystem::QueueTextureUpload(RenderUploadQueue& uploads, Texture& texture)
{
	GNS_PROFILE_FUNCTION();
	const Handle textureHandle = texture.GetHandle();
	if (!textureHandle.IsValid() || m_resourceCache.textures.contains(textureHandle))
	{
		return false;
	}

	const auto pending = std::find_if(
		uploads.textureUploads.begin(),
		uploads.textureUploads.end(),
		[textureHandle](const PendingTextureUpload& upload)
		{
			return upload.textureHandle == textureHandle;
		});
	if (pending != uploads.textureUploads.end())
	{
		return true;
	}

	uploads.textureUploads.emplace_back(PendingTextureUpload
	{
		.textureHandle = textureHandle,
		.texture = &texture
	});
	return true;
}

bool gns::RenderSystem::QueueShaderUpload(Shader& shader)
{
	GNS_PROFILE_FUNCTION();
	return QueueShaderUpload(m_pendingUploads, shader);
}

bool gns::RenderSystem::QueueShaderUpload(RenderUploadQueue& uploads, Shader& shader)
{
	GNS_PROFILE_FUNCTION();
	const Handle shaderHandle = shader.GetHandle();
	if (!shaderHandle.IsValid() || m_resourceCache.shaders.contains(shaderHandle))
	{
		return false;
	}

	const auto pending = std::find_if(
		uploads.shaderUploads.begin(),
		uploads.shaderUploads.end(),
		[shaderHandle](const PendingShaderUpload& upload)
		{
			return upload.shaderHandle == shaderHandle;
		});
	if (pending != uploads.shaderUploads.end())
	{
		return true;
	}

	uploads.shaderUploads.emplace_back(PendingShaderUpload
	{
		.shaderHandle = shaderHandle,
		.shader = &shader
	});
	return true;
}

bool gns::RenderSystem::QueueMaterialUpload(Material& material)
{
	GNS_PROFILE_FUNCTION();
	const Handle materialHandle = material.GetHandle();
	if (!materialHandle.IsValid())
	{
		return false;
	}

	const auto pending = std::find_if(
		m_pendingUploads.materialUploads.begin(),
		m_pendingUploads.materialUploads.end(),
		[materialHandle](const PendingMaterialUpload& upload)
		{
			return upload.materialHandle == materialHandle;
		});
	if (pending != m_pendingUploads.materialUploads.end())
	{
		return true;
	}

	m_pendingUploads.materialUploads.emplace_back(PendingMaterialUpload
	{
		.materialHandle = materialHandle,
		.material = &material
	});
	return true;
}

gns::RenderUploadQueue gns::RenderSystem::ConsumePendingRenderUploads()
{
	GNS_PROFILE_FUNCTION();
	RenderUploadQueue uploads = std::move(m_pendingUploads);
	m_pendingUploads = {};
	return uploads;
}

void gns::RenderSystem::HarvestCompletedRenderSubmissions()
{
	GNS_PROFILE_FUNCTION();
	m_renderThread.WaitForIdle();
	m_renderThread.DrainCompletedSubmissions(
		[this](RenderSubmission& completedSubmission)
		{
			RequeuePendingRenderUploads(completedSubmission.uploads);
		});
}

void gns::RenderSystem::RequeuePendingRenderUploads(RenderUploadQueue& uploads)
{
	GNS_PROFILE_FUNCTION();
	m_pendingUploads.meshUploads.insert(
		m_pendingUploads.meshUploads.end(),
		std::make_move_iterator(uploads.meshUploads.begin()),
		std::make_move_iterator(uploads.meshUploads.end()));
	m_pendingUploads.textureUploads.insert(
		m_pendingUploads.textureUploads.end(),
		std::make_move_iterator(uploads.textureUploads.begin()),
		std::make_move_iterator(uploads.textureUploads.end()));
	m_pendingUploads.shaderUploads.insert(
		m_pendingUploads.shaderUploads.end(),
		std::make_move_iterator(uploads.shaderUploads.begin()),
		std::make_move_iterator(uploads.shaderUploads.end()));
	m_pendingUploads.materialUploads.insert(
		m_pendingUploads.materialUploads.end(),
		std::make_move_iterator(uploads.materialUploads.begin()),
		std::make_move_iterator(uploads.materialUploads.end()));
	uploads.Clear();
}

void gns::RenderSystem::FlushRenderUploads(RenderUploadQueue& uploads)
{
	GNS_PROFILE_FUNCTION();
	for (const PendingShaderUpload& upload : uploads.shaderUploads)
	{
		GNS_PROFILE_SCOPE("RenderSystem::FlushShaderUpload");
		if (!upload.shaderHandle.IsValid() ||
			upload.shader == nullptr ||
			m_resourceCache.shaders.contains(upload.shaderHandle))
		{
			continue;
		}

		const Handle renderShaderHandle = m_renderer.CreateVulkanShader(*upload.shader);
		if (renderShaderHandle.IsValid())
		{
			m_resourceCache.shaders[upload.shaderHandle] = renderShaderHandle;
		}
	}
	uploads.shaderUploads.clear();

	for (const PendingMeshUpload& upload : uploads.meshUploads)
	{
		GNS_PROFILE_SCOPE("RenderSystem::FlushMeshUpload");
		if (!upload.meshHandle.IsValid() ||
			upload.mesh == nullptr ||
			m_resourceCache.meshes.contains(upload.meshHandle))
		{
			continue;
		}

		const Handle renderMeshHandle = m_renderer.ApplyMesh(*upload.mesh);
		if (renderMeshHandle.IsValid())
		{
			m_resourceCache.meshes[upload.meshHandle] = renderMeshHandle;
			upload.mesh->FreeCPUSide();
		}
	}
	uploads.meshUploads.clear();

	for (const PendingTextureUpload& upload : uploads.textureUploads)
	{
		GNS_PROFILE_SCOPE("RenderSystem::FlushTextureUpload");
		if (!upload.textureHandle.IsValid() ||
			upload.texture == nullptr ||
			m_resourceCache.textures.contains(upload.textureHandle))
		{
			continue;
		}

		const Handle renderTextureHandle = m_renderer.ApplyTexture(*upload.texture);
		if (renderTextureHandle.IsValid())
		{
			m_resourceCache.textures[upload.textureHandle] = renderTextureHandle;
			upload.texture->FreeCPUSide();
		}
	}
	uploads.textureUploads.clear();

	std::vector<PendingMaterialUpload> remainingMaterialUploads;
	for (const PendingMaterialUpload& upload : uploads.materialUploads)
	{
		GNS_PROFILE_SCOPE("RenderSystem::FlushMaterialUpload");
		if (!upload.materialHandle.IsValid() || upload.material == nullptr)
		{
			continue;
		}

		Material& material = *upload.material;
		if (!material.shader_ref.m_handle.IsValid())
		{
			if (m_resourceCache.materials.contains(upload.materialHandle))
			{
				continue;
			}

			remainingMaterialUploads.emplace_back(upload);
			continue;
		}

		Shader* shader = Object::Get<Shader>(material.shader_ref.m_handle);
		if (shader == nullptr)
		{
			remainingMaterialUploads.emplace_back(upload);
			continue;
		}

		const Handle shaderHandle = shader->GetHandle();
		if (!m_resourceCache.shaders.contains(shaderHandle))
		{
			QueueShaderUpload(uploads, *shader);
			remainingMaterialUploads.emplace_back(upload);
			continue;
		}

		const Handle renderShaderHandle = GetRenderShaderHandle(shaderHandle);
		rendering::VulkanShader* vulkanShader = renderShaderHandle.IsValid()
			? m_renderer.GetVulkanShader(renderShaderHandle)
			: nullptr;
		if (vulkanShader == nullptr)
		{
			remainingMaterialUploads.emplace_back(upload);
			continue;
		}

		const MaterialLayout& shaderMaterialLayout = vulkanShader->GetMaterialLayout();
		if (!material.GetLayout().IsCompatibleWith(shaderMaterialLayout))
		{
			material.SetLayout(shaderMaterialLayout, true);
			if (assets::AssetSystem* assetSystem = GetAssetSystem())
			{
				assetSystem->ApplyImportedMaterialDefaults(material);
			}
		}

		if (!ApplyMaterial(material, *vulkanShader, &uploads).IsValid())
		{
			remainingMaterialUploads.emplace_back(upload);
		}
	}
	uploads.materialUploads = std::move(remainingMaterialUploads);
}

void gns::RenderSystem::ExecuteRenderSubmission(RenderSubmission& submission)
{
	GNS_PROFILE_FUNCTION();
	FlushRenderUploads(submission.uploads);
	m_renderer.DrawFrame(submission.packet);
}

gns::Handle gns::RenderSystem::ApplyShader(Shader& shader)
{
	GNS_PROFILE_FUNCTION();
	const Handle shaderHandle = shader.GetHandle();
	if (const auto it = m_resourceCache.shaders.find(shaderHandle); it != m_resourceCache.shaders.end())
	{
		return it->second;
	}

	QueueShaderUpload(shader);
	return {};
}

gns::Handle gns::RenderSystem::ApplyTexture(Texture& texture)
{
	GNS_PROFILE_FUNCTION();
	const Handle textureHandle = texture.GetHandle();
	if (const auto it = m_resourceCache.textures.find(textureHandle); it != m_resourceCache.textures.end())
	{
		return it->second;
	}

	QueueTextureUpload(texture);
	return {};
}

gns::Handle gns::RenderSystem::ApplyMaterial(Material& material)
{
	GNS_PROFILE_FUNCTION();
	const Handle materialHandle = material.GetHandle();
	QueueMaterialUpload(material);

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
			else
			{
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
					}
				}
			}
		}
	}

	if (const auto it = m_resourceCache.materials.find(materialHandle); it != m_resourceCache.materials.end())
	{
		return it->second;
	}

	return {};
}

gns::Handle gns::RenderSystem::ApplyMaterial(
	Material& material,
	rendering::VulkanShader& vulkanShader,
	RenderUploadQueue* dependencyUploads)
{
	GNS_PROFILE_FUNCTION();
	const Handle materialHandle = material.GetHandle();

	auto ensureTextureApplied = [&](Handle textureHandle) -> bool
	{
		GNS_PROFILE_SCOPE("RenderSystem::ApplyMaterial::EnsureTextureApplied");
		if (!textureHandle.IsValid() || m_resourceCache.textures.contains(textureHandle))
		{
			return true;
		}

		Texture* texture = Object::Get<Texture>(textureHandle);
		if (texture == nullptr)
		{
			if (assets::AssetSystem* assetSystem = GetAssetSystem())
			{
				texture = assetSystem->EnsureTextureLoaded(textureHandle);
			}
		}

		if (texture == nullptr)
		{
			return false;
		}

		if (dependencyUploads != nullptr)
		{
			QueueTextureUpload(*dependencyUploads, *texture);
			return false;
		}

		return ApplyTexture(*texture).IsValid();
	};

	std::vector<MaterialTextureBinding> textureBindings;
	textureBindings.reserve(material.GetTextureSlotCount());
	for (size_t textureIndex = 0; textureIndex < material.GetTextureSlotCount(); ++textureIndex)
	{
		GNS_PROFILE_SCOPE("RenderSystem::ApplyMaterial::TextureSlot");
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
			textureHandle = GetDefaultTextureHandle(GetDefaultTextureForMaterialSlot(property->name));
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
	GNS_PROFILE_FUNCTION();
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
	GNS_PROFILE_FUNCTION();
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
	GNS_PROFILE_FUNCTION();
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
	GNS_PROFILE_FUNCTION();
	switch (texture)
	{
	case DefaultTexture::White:
		return m_defaultTextures.white;
	case DefaultTexture::Grey:
		return m_defaultTextures.grey;
	case DefaultTexture::Black:
		return m_defaultTextures.black;
	case DefaultTexture::Normal:
		return m_defaultTextures.normal;
	case DefaultTexture::ErrorCheckerboard:
		return m_defaultTextures.errorCheckerboard;
	default:
		LOG_WARNING("[RenderSystem]: Unknown default texture requested.");
		return {};
	}
}

gns::RenderTextureBinding gns::RenderSystem::GetTextureBinding(Handle textureHandle)
{
	GNS_PROFILE_FUNCTION();
	HarvestCompletedRenderSubmissions();

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
	GNS_PROFILE_FUNCTION();
	return GetTextureBinding(textureHandle).descriptor;
}

uint64_t gns::RenderSystem::GetSceneTextureDescriptor()
{
	GNS_PROFILE_FUNCTION();
	HarvestCompletedRenderSubmissions();
	return m_renderer.GetSceneTextureDescriptor();
}

void gns::RenderSystem::SetScreen(const Screen& screen)
{
	GNS_PROFILE_FUNCTION();
	HarvestCompletedRenderSubmissions();
	m_renderer.SetScreen(screen);
}

void gns::RenderSystem::CreateDefaultTextureObjects()
{
	GNS_PROFILE_FUNCTION();
	const rendering::VulkanDefaultTextureHandles& vulkanDefaults = m_renderer.GetDefaultTextures();

	m_defaultTextures.white = RegisterDefaultTexture(DefaultResourceNames::WhiteTexture, vulkanDefaults.white);
	m_defaultTextures.grey = RegisterDefaultTexture(DefaultResourceNames::GreyTexture, vulkanDefaults.grey);
	m_defaultTextures.black = RegisterDefaultTexture(DefaultResourceNames::BlackTexture, vulkanDefaults.black);
	m_defaultTextures.normal = RegisterDefaultTexture("default_normal_texture", vulkanDefaults.normal);
	m_defaultTextures.errorCheckerboard = RegisterDefaultTexture(
		DefaultResourceNames::ErrorCheckerboardTexture,
		vulkanDefaults.errorCheckerboard);
}

gns::Handle gns::RenderSystem::RegisterDefaultTexture(const char* name, Handle vulkanTextureHandle)
{
	GNS_PROFILE_FUNCTION();
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
	GNS_PROFILE_FUNCTION();
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
		m_defaultMeshMaterial = material->GetHandle();
	}
	QueueMaterialUpload(*material);

	return true;
}

gns::Handle gns::RenderSystem::GetDefaultMeshShaderHandle() const
{
	GNS_PROFILE_FUNCTION();
	return m_defaultMeshShader;
}

gns::Handle gns::RenderSystem::GetDefaultMeshMaterialHandle() const
{
	GNS_PROFILE_FUNCTION();
	return m_defaultMeshMaterial;
}

void gns::RenderSystem::BuildDrawData()
{
	GNS_PROFILE_FUNCTION();
	std::vector<DrawData>& drawDataList = m_framePacket.drawData;
	const size_t previousDrawCount = drawDataList.size();
	drawDataList.clear();
	drawDataList.reserve(previousDrawCount);
	m_modelMatrices.clear();
	m_modelMatrices.reserve(previousDrawCount);
	if (!EnsureDefaultMeshResources())
	{
		return;
	}
	assets::AssetSystem* assetSystem = GetAssetSystem();

	Shader* defaultShader = Object::Get<Shader>(m_defaultMeshShader);
	if (defaultShader == nullptr)
	{
		LOG_ERROR("[RenderSystem]: Cannot build draw data because default mesh shader is missing.");
		return;
	}
	uint32_t index = 0;
	core::SystemsManager::ForEach<SceneMemberComponent, Transform, MeshComponent>([&](
		const SceneMemberComponent& sceneMember,
		const Transform& transform,
		const MeshComponent& meshComp)
	{
		GNS_PROFILE_SCOPE("RenderSystem::BuildDrawData::MeshEntity");
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
			if (assetSystem != nullptr)
			{
				if (Mesh* mesh = assetSystem->EnsureMeshLoaded(meshComp.mesh.m_handle))
				{
					ApplyMesh(*mesh);
				}
			}
		}

		Material* material = Object::Get<Material>(meshComp.material.m_handle);
		if (material == nullptr && assetSystem != nullptr)
		{
			material = assetSystem->EnsureMaterialLoaded(meshComp.material.m_handle);
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
			return;
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
			if (assetSystem != nullptr)
			{
				assetSystem->ApplyImportedMaterialDefaults(*material);
			}
		}

		const Handle renderMaterialHandle = ApplyMaterial(*material);
		if (!renderMaterialHandle.IsValid())
		{
			return;
		}

		rendering::VulkanMaterial* vulkanMaterial = m_renderer.GetVulkanMaterial(renderMaterialHandle);
		if (vulkanMaterial == nullptr)
		{
			return;
		}

		m_modelMatrices.emplace_back(transform.matrix);
		DrawData drawData;
		drawData.transform = transform.matrix;
		drawData.vkShader = vulkanShader;
		drawData.vk_indexBuffer = vulkanMesh->indexBuffer.buffer;
		drawData.vkMaterial = vulkanMaterial;
		drawData.vk_vertexBufferAddress = vulkanMesh->vertexBufferAddress;
		drawData.StartIndex = vulkanMesh->startIndex;
		drawData.Count = vulkanMesh->count;
		drawData.index = index;
		drawDataList.emplace_back(drawData);
		index++;
	});
}

void gns::RenderSystem::BuildGlobalFrameDataDescriptor()
{
	GNS_PROFILE_FUNCTION();
	m_sceneData = SceneData();
	m_sceneData.view = m_renderer.m_cameraBackend.view;
	m_sceneData.proj = m_renderer.m_cameraBackend.projection;
	m_sceneData.viewproj = m_renderer.m_cameraBackend.viewProjection;
	m_directionalLights = DirectionalLightBuffer();
	m_pointLights = PointLightBuffer();
	m_spotLights = SpotLightBuffer();

	bool hasAmbientLight = false;
	core::SystemsManager::ForEach<SceneMemberComponent, AmbientLightComponent>([&](
		const SceneMemberComponent& sceneMember,
		const AmbientLightComponent& ambientLight)
	{
		GNS_PROFILE_SCOPE("RenderSystem::BuildGlobalFrameDataDescriptor::AmbientLight");
		if (hasAmbientLight || !SceneManager::IsSceneLoaded(sceneMember.scene_handle))
		{
			return;
		}

		m_sceneData.ambientColor = ambientLight.color;
		hasAmbientLight = true;
	});

	bool directionalLightLimitReached = false;
	core::SystemsManager::ForEach<SceneMemberComponent, DirectionalLightComponent, Transform>([&](
		const SceneMemberComponent& sceneMember,
		DirectionalLightComponent& directionalLight, 
		Transform& transform)
	{
		GNS_PROFILE_SCOPE("RenderSystem::BuildGlobalFrameDataDescriptor::DirectionalLight");
		if (!SceneManager::IsSceneLoaded(sceneMember.scene_handle))
		{
			return;
		}

		if (m_directionalLights.count >= MaxSceneLights)
		{
			if (!directionalLightLimitReached)
			{
				LOG_WARNING("[RenderSystem]: Directional light count exceeded MaxSceneLights. Extra lights are ignored.");
				directionalLightLimitReached = true;
			}
			return;
		}

		DirectionalLightGpu& light = m_directionalLights.lights[m_directionalLights.count++];
		light.direction = {TransformHelper::Forward(transform),directionalLight.intensity};
		light.color = {directionalLight.color, 0};
	});

	bool pointLightLimitReached = false;
	core::SystemsManager::ForEach<SceneMemberComponent, Transform, PointLightComponent>([&](
		const SceneMemberComponent& sceneMember,
		const Transform& transform,
		const PointLightComponent& pointLight)
	{
		GNS_PROFILE_SCOPE("RenderSystem::BuildGlobalFrameDataDescriptor::PointLight");
		if (!SceneManager::IsSceneLoaded(sceneMember.scene_handle))
		{
			return;
		}

		if (m_pointLights.count >= MaxSceneLights)
		{
			if (!pointLightLimitReached)
			{
				LOG_WARNING("[RenderSystem]: Point light count exceeded MaxSceneLights. Extra lights are ignored.");
				pointLightLimitReached = true;
			}
			return;
		}

		PointLightGpu& light = m_pointLights.lights[m_pointLights.count++];
		light.position = glm::vec4(transform.position, pointLight.range);
		light.color = glm::vec4(glm::vec3(pointLight.color), pointLight.intensity);
	});

	bool spotLightLimitReached = false;
	core::SystemsManager::ForEach<SceneMemberComponent, Transform, SpotLightComponent>([&](
		const SceneMemberComponent& sceneMember,
		const Transform& transform,
		const SpotLightComponent& spotLight)
	{
		GNS_PROFILE_SCOPE("RenderSystem::BuildGlobalFrameDataDescriptor::SpotLight");
		if (!SceneManager::IsSceneLoaded(sceneMember.scene_handle))
		{
			return;
		}

		if (m_spotLights.count >= MaxSceneLights)
		{
			if (!spotLightLimitReached)
			{
				LOG_WARNING("[RenderSystem]: Spot light count exceeded MaxSceneLights. Extra lights are ignored.");
				spotLightLimitReached = true;
			}
			return;
		}

		SpotLightGpu& light = m_spotLights.lights[m_spotLights.count++];
		light.position = glm::vec4(transform.position, spotLight.range);
		light.direction = spotLight.direction;
		light.color = glm::vec4(glm::vec3(spotLight.color), spotLight.intensity);
		light.cone = glm::vec4(
			glm::cos(glm::radians(spotLight.innerAngle)),
			glm::cos(glm::radians(spotLight.outerAngle)),
			0.0f,
			0.0f);
	});

	m_globalFrameDataDescriptor.sceneData = GpuDataDescriptor::GetFromType<gns::SceneData>(&m_sceneData);
	m_globalFrameDataDescriptor.directionalLights = GpuDataDescriptor::GetFromType(&m_directionalLights);
	m_globalFrameDataDescriptor.pointLights = GpuDataDescriptor::GetFromType(&m_pointLights);
	m_globalFrameDataDescriptor.spotLights = GpuDataDescriptor::GetFromType(&m_spotLights);
	m_hasGlobalFrameDataDescriptor = true;
}
