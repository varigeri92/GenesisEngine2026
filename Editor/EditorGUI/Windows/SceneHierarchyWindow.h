#pragma once
#include <string>
#include <utility>

#include "GenesisGUI.h"

class SceneHierarchyWindow : public GuiWindow
{
public:
    explicit SceneHierarchyWindow(std::string title) : GuiWindow(std::move(title)) {}

    void OnDraw() override;
};
