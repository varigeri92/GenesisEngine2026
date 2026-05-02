#include "SceneViewWindow.h"

#include "Genesis.h"
#include "../../../Engine/Systems/GuiSystem.h"

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
}
