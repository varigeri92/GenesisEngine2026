#include "gnspch.h"
#include "TestWindow.h"

#include "imgui.h"

TestWindow::~TestWindow(){}

void TestWindow::OnDraw()
{
    ImGui::Begin(Title.c_str());
    ImGui::Text("Hello");
    ImGui::End();
}
