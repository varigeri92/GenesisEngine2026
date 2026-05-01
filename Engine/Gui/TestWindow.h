#pragma once
#include "GuiWindow.h"
#include "imgui.h"
#include "../Core/Handles.h"
#include "../Object/Texture.h"

class TestWindow : public GuiWindow
{
    gns::Reference<gns::Texture> m_checkerboardTexture;
    bool m_checkerboardTextureLoadAttempted = false;
public:
    TestWindow(std::string title): GuiWindow(title){}
    ~TestWindow() override;
    void OnDraw() override;
private:
    void LoadCheckerboardTexture();
};
