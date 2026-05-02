#pragma once
#include <string>
#include <utility>

#include "GenesisGUI.h"

class SceneViewWindow : public GuiWindow
{
public:
	explicit SceneViewWindow(std::string title) : GuiWindow(std::move(title)) {}

	void OnDraw() override;
};
