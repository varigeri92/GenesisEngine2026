#include "gnspch.h"
#include "TestWindow.h"

#include "imgui.h"

TestWindow::~TestWindow() = default;

void TestWindow::OnDraw()
{
    ImGui::Text("Hello");
}
