#include "SystemViewer.h"

#include "../../../Engine/Utils/Time.h"



SystemViewer::SystemViewer(const std::string& title) : GuiWindow(title)
{
}

void SystemViewer::BeginWindow()
{
    GuiWindow::BeginWindow();
}

void SystemViewer::OnDraw()
{
    
    const double deltaSeconds = static_cast<double>(gns::Time::DeltaTime());

    const double frameTimeMs = deltaSeconds * 1000.0;
    const double fps = deltaSeconds > 0.0 ? 1.0 / deltaSeconds : 0.0;

    m_frameTimeMs.AddSample(frameTimeMs, 0.0001);
    m_fps.AddSample(fps, 0.0001);
    
    ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_BordersOuter;
    using Systems = gns::core::SystemsManager;
    if (ImGui::BeginTable("Table_frame_info", 2, flags))
    {
        ImGui::TableNextColumn();
        ImGui::Text("Frame Time:");
        ImGui::TableNextColumn();
        ImGui::Text("%.3f ms", m_frameTimeMs.value);
        ImGui::TableNextRow();
        
        ImGui::TableNextColumn();
        ImGui::Text("FPS:");
        ImGui::TableNextColumn();
        ImGui::Text("%.1f", m_fps.value);
        ImGui::TableNextRow();
        
        ImGui::TableNextColumn();
        ImGui::Text("Systems Runnig:");
        ImGui::TableNextColumn();
        ImGui::Text("%u", Systems::Systems.size());
        ImGui::TableNextRow();
        
        ImGui::EndTable();
    }
    
    
    if (ImGui::BeginTable("Table", 5, flags))
    {
        ImGui::TableNextColumn();
        ImGui::Text("Index");
        ImGui::TableNextColumn();
        ImGui::Text("Name");
        ImGui::TableNextColumn();
        ImGui::Text("Update");
        ImGui::TableNextColumn();
        ImGui::Text("LateUpdate");
        ImGui::TableNextColumn();
        ImGui::Text("State");
        ImGui::TableNextRow();
        for (size_t i = 0; i < Systems::Systems.size(); i++)
        {
            ImGui::TableNextColumn();
            ImGui::Text("%u", i);
            ImGui::TableNextColumn();
            ImGui::Text("%s", Systems::Systems[i]->metadata.name.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%.3f ms", Systems::Systems[i]->metadata._updateTimer.smoothedTimeMs);
            ImGui::TableNextColumn();
            ImGui::Text("%.3f ms", Systems::Systems[i]->metadata._lateTimer.smoothedTimeMs);
            ImGui::TableNextColumn();
            ImGui::Text("%s", SystemStateToString(Systems::Systems[i]->State).data());
            ImGui::TableNextRow();
        }
        ImGui::EndTable();
    }
}

void SystemViewer::EndWindow()
{
    GuiWindow::EndWindow();
}
