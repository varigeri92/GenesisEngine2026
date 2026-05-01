#include "gnspch.h"
#include "TestWindow.h"

#include "../Systems/GuiSystem.h"
#include "../Systems/SystemsManager.h"

TestWindow::~TestWindow() = default;

void TestWindow::OnDraw()
{
    ImGui::Text("Hello");

    if (!m_checkerboardTextureLoadAttempted)
    {
        LoadCheckerboardTexture();
    }

    if (m_checkerboardTexture != 0)
    {
        ImGui::Text("Default checkerboard");
        ImGui::Image(ImTextureRef(static_cast<ImTextureID>(m_checkerboardTexture)), ImVec2(256.0f, 256.0f));
    }
    else
    {
        ImGui::Text("Checkerboard texture unavailable");
    }
}

void TestWindow::LoadCheckerboardTexture()
{
    m_checkerboardTextureLoadAttempted = true;

    GuiSystem* guiSystem = gns::core::SystemsManager::GetSystem<GuiSystem>();
    if (guiSystem == nullptr)
    {
        return;
    }

    m_checkerboardTexture = guiSystem->GetDefaultCheckerboardTexture();
}
