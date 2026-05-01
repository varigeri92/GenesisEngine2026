#pragma once
#include <string>
#include <utility>

#include "GenesisGUI.h"

class IconBrowserWindow : public GuiWindow
{
public:
    explicit IconBrowserWindow(std::string title) : GuiWindow(std::move(title)) {}

    void OnDraw() override;
};
