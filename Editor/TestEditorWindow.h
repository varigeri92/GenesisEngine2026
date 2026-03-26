#pragma once
#include <string>

#include "GenesisGUI.h"

class TestEditorWindow : public GuiWindow
{
public:
    explicit TestEditorWindow(std::string title): GuiWindow(title){}

    void OnDraw() override;
};
