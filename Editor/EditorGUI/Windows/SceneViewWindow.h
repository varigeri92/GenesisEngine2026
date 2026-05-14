#pragma once
#include <string>
#include <utility>

#include "../../Assets/ModelImportController.h"
#include "GenesisGUI.h"

class SceneViewWindow : public GuiWindow
{
	enum class GizmoOperation
	{
		translate, rotate, scale, box
	};
	
public:
	explicit SceneViewWindow(std::string title) : GuiWindow(std::move(title)) {}

	void OnDraw() override;

private:
	editor::assets::ModelImportController m_modelImportController = {};
	GizmoOperation gizmoMode = GizmoOperation::translate;
	bool local = false;
	void AcceptSceneAssetDrop();
	void DrawTransformGizmo(const ImVec2& scenePosition, const ImVec2& sceneSize);
public:
	void BeginWindow() override;
	void EndWindow() override;
};
