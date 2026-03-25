#pragma once
#include "GuiWindow.h"

class TestWindow : public GuiWindow
{
public:
    TestWindow(std::string title): GuiWindow(title){}
    ~TestWindow() override;
    void OnDraw() override;
};
