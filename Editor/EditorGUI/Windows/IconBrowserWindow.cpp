#include "IconBrowserWindow.h"

#include <string>

#include "GenesisMaterialIcons.h"

namespace
{
    bool MatchesFilter(const GenesisMaterialIconDefinition& icon, const char* filter)
    {
        if (filter == nullptr || filter[0] == '\0')
        {
            return true;
        }

        const std::string needle = filter;
        return std::string(icon.Name).find(needle) != std::string::npos ||
            std::string(icon.Macro).find(needle) != std::string::npos;
    }
}

void IconBrowserWindow::OnDraw()
{
    static char filter[128] = {};

    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##IconFilter", "Search icons or macros", filter, sizeof(filter));
    ImGui::Text("Material Icons Round: %d icons", GenesisMaterialIconCount);

    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_Reorderable |
        ImGuiTableFlags_ScrollY;

    const float tableHeight = ImGui::GetContentRegionAvail().y;
    if (ImGui::BeginTable("MaterialIconTable", 4, tableFlags, ImVec2(0.0f, tableHeight)))
    {
        ImGui::TableSetupColumn("Icon", ImGuiTableColumnFlags_WidthFixed, 48.0f);
        ImGui::TableSetupColumn("Icon name", ImGuiTableColumnFlags_WidthStretch, 0.36f);
        ImGui::TableSetupColumn("MacroDefinition", ImGuiTableColumnFlags_WidthStretch, 0.42f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.22f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        for (int index = 0; index < GenesisMaterialIconCount; ++index)
        {
            const GenesisMaterialIconDefinition& icon = GenesisMaterialIcons[index];
            if (!MatchesFilter(icon, filter))
            {
                continue;
            }

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(icon.Icon);

            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(icon.Name);

            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(icon.Macro);

            ImGui::TableSetColumnIndex(3);
            ImGui::TextUnformatted(icon.Value);
        }

        ImGui::EndTable();
    }
}
