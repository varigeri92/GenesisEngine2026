#include "DockingRoot.h"

#include <functional>
#include <string>
#include <utility>
#include "GenesisMaterialIcons.h"
#include "GenesisWindow.h"
#include "../../../Engine/Scene/SceneManager.h"
#include "../../../Engine/Scene/SceneSerializer.h"

namespace
{
    constexpr float TitleBarHeight = 28.0f;
    constexpr float TitleBarButtonWidth = 34.0f;

    void DrawTitleButton(const char* label, const ImVec2& size, const std::function<void()>& action)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
        if (ImGui::Button(label, size))
        {
            action();
        }
        ImGui::PopStyleVar(2);
    }

    void DrawTitleBar(const char* title)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        const ImVec2 windowPos = ImGui::GetWindowPos();
        const ImVec2 windowSize = ImGui::GetWindowSize();
        const ImVec2 titleBarMin = windowPos;
        const ImVec2 titleBarMax = ImVec2(windowPos.x + windowSize.x, windowPos.y + TitleBarHeight);
        drawList->AddRectFilled(titleBarMin, titleBarMax, ImGui::GetColorU32(ImGuiCol_TitleBgActive));

        ImGui::SetCursorScreenPos(ImVec2(titleBarMin.x + 10.0f, titleBarMin.y + 6.0f));
        ImGui::TextUnformatted(title);

        const float buttonHeight = TitleBarHeight;
        ImGui::SetCursorScreenPos(ImVec2(titleBarMax.x - (TitleBarButtonWidth * 3.0f), titleBarMin.y));

        DrawTitleButton(ICON_MD_MINIMIZE "##MinimizeWindow", ImVec2(TitleBarButtonWidth, buttonHeight), []()
        {
            gns::window::MinimizeMainWindow();
        });

        ImGui::SameLine(0.0f, 0.0f);
        const char* maximizeIcon = gns::window::IsMainWindowMaximized() ? ICON_MD_FULLSCREEN_EXIT : ICON_MD_FULLSCREEN;
        const std::string maximizeLabel = std::string(maximizeIcon) + "##MaximizeWindow";
        DrawTitleButton(maximizeLabel.c_str(), ImVec2(TitleBarButtonWidth, buttonHeight), []()
        {
            gns::window::ToggleMaximizeMainWindow();
        });

        ImGui::SameLine(0.0f, 0.0f);
        DrawTitleButton(ICON_MD_CLOSE "##CloseWindow", ImVec2(TitleBarButtonWidth, buttonHeight), []()
        {
            gns::window::RequestCloseMainWindow();
        });

        ImGui::SetCursorScreenPos(ImVec2(titleBarMin.x, titleBarMax.y));
    }

    void DrawMenuBar()
    {
        const float menuBarHeight = ImGui::GetFrameHeight();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        const ImVec2 windowPos = ImGui::GetWindowPos();
        const ImVec2 windowSize = ImGui::GetWindowSize();
        const ImVec2 menuMin = ImGui::GetCursorScreenPos();
        const ImVec2 menuMax = ImVec2(windowPos.x + windowSize.x, menuMin.y + menuBarHeight);
        drawList->AddRectFilled(menuMin, menuMax, ImGui::GetColorU32(ImGuiCol_MenuBarBg));

        ImGui::SetCursorScreenPos(menuMin);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 0.0f));

        constexpr ImGuiWindowFlags menuWindowFlags =
            ImGuiWindowFlags_MenuBar |
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::BeginChild("DockingRootMenuBar", ImVec2(0.0f, menuBarHeight), ImGuiChildFlags_None, menuWindowFlags);
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Exit"))
                {
                    gns::window::RequestCloseMainWindow();
                }
                
                if (ImGui::MenuItem("Save"))
                {
                    gns::SceneSerializer::SaveScene(gns::SceneManager::GetActiveScene());
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit"))
            {
                ImGui::MenuItem("Undo", "Ctrl+Z", false, false);
                ImGui::MenuItem("Redo", "Ctrl+Y", false, false);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Window"))
            {
                ImGui::MenuItem("Reset Layout", nullptr, false, false);
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
        ImGui::EndChild();

        ImGui::PopStyleVar(2);
        ImGui::SetCursorScreenPos(ImVec2(menuMin.x, menuMax.y));
    }
}

DockingRoot::DockingRoot(std::string title) : GuiWindow(std::move(title))
{
}

DockingRoot::~DockingRoot() = default;

void DockingRoot::BeginWindow()
{
    open = true;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    constexpr ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBackground;

    ImGui::Begin(Title.c_str(), nullptr, windowFlags);
}

void DockingRoot::OnDraw()
{
    DrawTitleBar("Genesis");
    DrawMenuBar();

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

    ImGuiID dockspaceId = ImGui::GetID("GenesisEditorDockspace");
    constexpr ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);

    ImGui::PopStyleColor(2);
}

void DockingRoot::EndWindow()
{
    ImGui::End();
    ImGui::PopStyleVar(3);
}
