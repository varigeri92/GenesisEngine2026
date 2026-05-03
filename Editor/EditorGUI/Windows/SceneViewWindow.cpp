#include "SceneViewWindow.h"

#include <yaml-cpp/yaml.h>

#include "../../EditorAssetDragDrop.h"
#include "Genesis.h"
#include "../../../Engine/Assets/AssetManager.h"
#include "../../../Engine/Renderer/RenderSystem.h"
#include "../../../Engine/Systems/GuiSystem.h"

namespace
{
	constexpr const char* ModelImportPopupTitle = "Import Model";

	std::string AssetTypeToString(gns::assets::AssetType assetType)
	{
		switch (assetType)
		{
		case gns::assets::Mesh:
			return "Mesh";
		case gns::assets::Texture:
			return "Texture";
		case gns::assets::Shader:
			return "Shader";
		case gns::assets::Material:
			return "Material";
		default:
			return "Generic";
		}
	}
}

void SceneViewWindow::AcceptSceneAssetDrop()
{
	if (!ImGui::BeginDragDropTarget())
	{
		return;
	}

	const ImGuiPayload* payload =
		ImGui::AcceptDragDropPayload(editor::dragdrop::ProjectAssetPayloadType);
	if (payload != nullptr)
	{
		if (payload->DataSize != sizeof(editor::dragdrop::ProjectAssetPayload))
		{
			LOG_WARNING("[SceneViewWindow]: Rejected project asset drop with invalid payload size.");
		}
		else
		{
			const auto* assetPayload =
				static_cast<const editor::dragdrop::ProjectAssetPayload*>(payload->Data);
			if (assetPayload->assetType != gns::assets::Mesh)
			{
				LOG_WARNING("[SceneViewWindow]: Rejected project asset drop because it is not a mesh.");
				LOG_WARNING(assetPayload->path);
			}
			else
			{
				BeginModelImport(assetPayload->path);
			}
		}
	}

	ImGui::EndDragDropTarget();
}

void SceneViewWindow::BeginModelImport(const std::filesystem::path& assetPath)
{
	m_pendingModelImportPath = gns::path::Normalize(assetPath);
	m_modelImportOptions = {};
	m_modelImportError.clear();
	m_modelImportPopupOpen = true;
	m_shouldOpenModelImportPopup = true;
}

void SceneViewWindow::DrawModelImportPopup()
{
	if (m_shouldOpenModelImportPopup)
	{
		ImGui::OpenPopup(ModelImportPopupTitle);
		m_shouldOpenModelImportPopup = false;
	}

	bool popupOpen = m_modelImportPopupOpen;
	if (ImGui::BeginPopupModal(ModelImportPopupTitle, &popupOpen, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextWrapped("%s", m_pendingModelImportPath.string().c_str());
		ImGui::Separator();

		ImGui::Checkbox("Flatten hierarchy", &m_modelImportOptions.flattenHierarchy);
		ImGui::Checkbox("Import skeleton", &m_modelImportOptions.importSkeleton);
		ImGui::Checkbox("Import materials", &m_modelImportOptions.importMaterials);
		ImGui::Checkbox("Import textures", &m_modelImportOptions.importTextures);

		if (!m_modelImportError.empty())
		{
			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.25f, 1.0f), "%s", m_modelImportError.c_str());
		}

		ImGui::Separator();
		if (ImGui::Button("Import"))
		{
			if (CompleteModelImport())
			{
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			CancelModelImport();
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	if (m_modelImportPopupOpen && !popupOpen)
	{
		CancelModelImport();
	}
}

void SceneViewWindow::CancelModelImport()
{
	m_pendingModelImportPath.clear();
	m_modelImportOptions = {};
	m_modelImportError.clear();
	m_modelImportPopupOpen = false;
	m_shouldOpenModelImportPopup = false;
}

bool SceneViewWindow::CompleteModelImport()
{
	if (m_pendingModelImportPath.empty())
	{
		m_modelImportError = "No model is pending import.";
		return false;
	}

	if (!WriteModelMetaFile())
	{
		m_modelImportError = "Failed to write model meta file.";
		return false;
	}

	gns::RenderSystem* renderSystem = gns::core::SystemsManager::GetSystem<gns::RenderSystem>();
	if (renderSystem == nullptr)
	{
		m_modelImportError = "RenderSystem is missing.";
		return false;
	}

	gns::assets::AssetLoadOptions loadOptions = {};
	loadOptions.flattenHierarchy = m_modelImportOptions.flattenHierarchy;
	loadOptions.importSkeleton = m_modelImportOptions.importSkeleton;
	loadOptions.importMaterials = m_modelImportOptions.importMaterials;
	loadOptions.importTextures = m_modelImportOptions.importTextures;

	if (!renderSystem->LoadMeshAssetIntoScene(m_pendingModelImportPath, loadOptions))
	{
		m_modelImportError = "Model import failed.";
		return false;
	}

	CancelModelImport();
	return true;
}

bool SceneViewWindow::WriteModelMetaFile() const
{
	const std::filesystem::path normalizedAssetPath = gns::path::Normalize(m_pendingModelImportPath);
	std::filesystem::path metaPath = normalizedAssetPath;
	metaPath += ".meta";

	const std::filesystem::path projectRoot = gns::path::ProjectDirectory();
	const std::string sourcePath = projectRoot.empty()
		? normalizedAssetPath.generic_string()
		: gns::path::ToRelative(normalizedAssetPath, projectRoot).generic_string();

	YAML::Emitter emitter;
	emitter << YAML::BeginMap;
	emitter << YAML::Key << "assetType" << YAML::Value << AssetTypeToString(gns::assets::Mesh);
	emitter << YAML::Key << "sourcePath" << YAML::Value << sourcePath;
	emitter << YAML::Key << "importerVersion" << YAML::Value << 1;
	emitter << YAML::Key << "importOptions" << YAML::Value << YAML::BeginMap;
	emitter << YAML::Key << "flattenHierarchy" << YAML::Value << m_modelImportOptions.flattenHierarchy;
	emitter << YAML::Key << "importSkeleton" << YAML::Value << m_modelImportOptions.importSkeleton;
	emitter << YAML::Key << "importMaterials" << YAML::Value << m_modelImportOptions.importMaterials;
	emitter << YAML::Key << "importTextures" << YAML::Value << m_modelImportOptions.importTextures;
	emitter << YAML::EndMap;
	emitter << YAML::EndMap;

	if (!emitter.good())
	{
		LOG_ERROR("[SceneViewWindow]: Failed to create model meta YAML.");
		return false;
	}

	if (!gns::path::WriteTextFile(metaPath, emitter.c_str()))
	{
		LOG_ERROR("[SceneViewWindow]: Failed to write model meta file.");
		LOG_ERROR(metaPath.string());
		return false;
	}

	LOG_INFO("[SceneViewWindow]: Wrote model meta file.");
	LOG_INFO(metaPath.string());
	return true;
}

void SceneViewWindow::OnDraw()
{
	GuiSystem* guiSystem = gns::core::SystemsManager::GetSystem<GuiSystem>();
	if (guiSystem == nullptr)
	{
		return;
	}

	const ImVec2 availableRegion = ImGui::GetContentRegionAvail();
	if (availableRegion.x <= 0.0f || availableRegion.y <= 0.0f)
	{
		return;
	}

	const ImVec2 scenePosition = ImGui::GetCursorScreenPos();
	gns::Screen sceneScreen(
		static_cast<uint32_t>(scenePosition.x),
		static_cast<uint32_t>(scenePosition.y),
		static_cast<uint32_t>(availableRegion.x),
		static_cast<uint32_t>(availableRegion.y));
	guiSystem->SetSceneScreen(sceneScreen);

	const uint64_t sceneTextureDescriptor = guiSystem->GetSceneTextureDescriptor();
	if (sceneTextureDescriptor == 0)
	{
		return;
	}

	ImGui::Image(
		ImTextureRef(static_cast<ImTextureID>(sceneTextureDescriptor)),
		availableRegion);
	AcceptSceneAssetDrop();
	DrawModelImportPopup();
}
