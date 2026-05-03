#pragma once
#include <filesystem>
#include <string>
#include <utility>

#include "GenesisGUI.h"

class SceneViewWindow : public GuiWindow
{
public:
	explicit SceneViewWindow(std::string title) : GuiWindow(std::move(title)) {}

	void OnDraw() override;

private:
	struct ModelImportOptions
	{
		bool flattenHierarchy = false;
		bool importSkeleton = true;
		bool importMaterials = true;
		bool importTextures = true;
	};

	std::filesystem::path m_pendingModelImportPath = {};
	ModelImportOptions m_modelImportOptions = {};
	std::string m_modelImportError = {};
	bool m_modelImportPopupOpen = false;
	bool m_shouldOpenModelImportPopup = false;

	void AcceptSceneAssetDrop();
	void BeginModelImport(const std::filesystem::path& assetPath);
	void DrawModelImportPopup();
	void CancelModelImport();
	bool CompleteModelImport();
	bool WriteModelMetaFile() const;
};
