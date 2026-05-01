#pragma once
#include <cstdint>

#include "GuiWindow.h"
#include "imgui.h"

class TestWindow : public GuiWindow
{
    uint64_t m_checkerboardTexture = 0;
    bool m_checkerboardTextureLoadAttempted = false;
public:
    TestWindow(std::string title): GuiWindow(title){}
    ~TestWindow() override;
    void OnDraw() override;
private:
    void LoadCheckerboardTexture();
};
