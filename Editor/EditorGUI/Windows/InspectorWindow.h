#pragma once
#include <string>
#include <utility>

#include "GenesisGUI.h"

class InspectorWindow : public GuiWindow
{
public:
    explicit InspectorWindow(std::string title) : GuiWindow(std::move(title)) {}

    void OnDraw() override;
};
