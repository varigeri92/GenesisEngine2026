#include "SceneViewWindow.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <ImGuizmo.h>
#include "../../EditorAssetDragDrop.h"
#include "../../EditorSelection.h"
#include "Genesis.h"
#include "GenesisMaterialIcons.h"
#include "../../../Engine/Assets/AssetManager.h"
#include "../../../Engine/Renderer/RenderSystem.h"
#include "../../../Engine/Systems/GuiSystem.h"

namespace
{
	glm::mat4 BuildImGuizmoProjection(const glm::mat4& cameraProjection)
	{
		glm::mat4 projection = cameraProjection;
		projection[1][1] = std::abs(projection[1][1]);

		const float depthA = cameraProjection[2][2];
		const float depthB = cameraProjection[3][2];
		if (depthA <= 0.0f || depthB <= 0.0f)
		{
			return projection;
		}

		const float nearPlane = depthB / (1.0f + depthA);
		const float farPlane = depthB / depthA;
		if (!std::isfinite(nearPlane) ||
			!std::isfinite(farPlane) ||
			nearPlane <= 0.0f ||
			farPlane <= nearPlane)
		{
			return projection;
		}

		projection[2][2] = farPlane / (nearPlane - farPlane);
		projection[3][2] = -(farPlane * nearPlane) / (farPlane - nearPlane);
		return projection;
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
				std::filesystem::path metaPath= assetPayload->path;
				metaPath+= ".meta";
				m_modelImportController.Begin(assetPayload->path);
				if (gns::path::Exists(metaPath))
				{
					LOG_INFO("Asset already imported!");
					m_modelImportController.LoadModel(metaPath);
				}
			}
		}
	}

	ImGui::EndDragDropTarget();
}

void SceneViewWindow::DrawTransformGizmo(const ImVec2& scenePosition, const ImVec2& sceneSize)
{
	if (EditorSelection::GetSelectionType() != EditorSelection::Type::Entity)
	{
		return;
	}

	const gns::entityHandle selectedEntity = EditorSelection::GetSelectedEntity();
	gns::Entity entity(selectedEntity);
	if (!entity.IsValid())
	{
		return;
	}

	Transform* transform = entity.TryGetComponent<Transform>();
	if (transform == nullptr)
	{
		return;
	}

	gns::RenderSystem* renderSystem = gns::core::SystemsManager::GetSystem<gns::RenderSystem>();
	if (renderSystem == nullptr)
	{
		return;
	}

	const CameraBackend& camera = renderSystem->GetCamera();
	glm::mat4 transformMatrix =
		glm::translate(glm::mat4(1.0f), transform->position) *
		glm::mat4_cast(glm::quat(glm::radians(transform->rotation))) *
		glm::scale(glm::mat4(1.0f), transform->scale);

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
	ImGuizmo::SetRect(scenePosition.x, scenePosition.y, sceneSize.x, sceneSize.y);

	glm::mat4 gizmoProjection = BuildImGuizmoProjection(camera.projection);

	ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
	switch (gizmoMode)
	{
	case GizmoOperation::translate:
		operation = ImGuizmo::TRANSLATE;
		break;
	case GizmoOperation::rotate:
		operation = ImGuizmo::ROTATE;
		break;
	case GizmoOperation::scale:
		operation = ImGuizmo::SCALE;
		break;
	case GizmoOperation::box:
		operation = ImGuizmo::BOUNDS;
		break;
	}
	ImGuizmo::MODE mode = local? ImGuizmo::LOCAL : ImGuizmo::WORLD;
	
	if (!ImGuizmo::Manipulate(
		glm::value_ptr(camera.view),
		glm::value_ptr(gizmoProjection),
		operation,
		mode,
		glm::value_ptr(transformMatrix)))
	{
		return;
	}

	float translation[3] = {};
	float rotation[3] = {};
	float scale[3] = {};
	ImGuizmo::DecomposeMatrixToComponents(
		glm::value_ptr(transformMatrix),
		translation,
		rotation,
		scale);

	transform->position = glm::vec3(translation[0], translation[1], translation[2]);
	transform->rotation = glm::vec3(rotation[0], rotation[1], rotation[2]);
	transform->scale = glm::vec3(scale[0], scale[1], scale[2]);
	transform->matrix = transformMatrix;
}

void SceneViewWindow::BeginWindow()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	GuiWindow::BeginWindow();
}

void SceneViewWindow::EndWindow()
{
	GuiWindow::EndWindow();
	ImGui::PopStyleVar();
}

void SceneViewWindow::OnDraw()
{
	if (ImGui::Button(ICON_MD_ARROW_OUTWARD))
	{
		gizmoMode = GizmoOperation::translate;
	}
	ImGui::SameLine();
	if (ImGui::Button(ICON_MD_3K))
	{
		gizmoMode = GizmoOperation::scale;
	}
	ImGui::SameLine();
	if (ImGui::Button(ICON_MD_ROTATE_LEFT))
	{
		gizmoMode = GizmoOperation::rotate;
	}
	
	ImGui::SameLine();
	if (ImGui::Button(ICON_MD_ADD_HOME))
	{
		local = !local;
	}
	
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
	DrawTransformGizmo(scenePosition, availableRegion);
	m_modelImportController.DrawPopup();
}
