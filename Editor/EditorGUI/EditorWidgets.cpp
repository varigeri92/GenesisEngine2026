#include "EditorWidgets.h"

#include "imgui.h"

constexpr float label_ratio = 0.35f;
static float available_Width;
static float label_width;

constexpr  ImGuiTableFlags widget_flags = ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_SizingFixedFit
| ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX;

bool widgets::startWidgets(const std::string& widgetGroupID)
{
	available_Width = ImGui::GetContentRegionAvail().x;
	label_width = available_Width * label_ratio;
	bool table_bool = ImGui::BeginTable(("##" + widgetGroupID).c_str(), 2, widget_flags);
	if (table_bool)
	{
	    ImGui::TableSetupColumn("##", ImGuiTableColumnFlags_WidthFixed, label_width);
	    ImGui::TableSetupColumn("##", ImGuiTableColumnFlags_WidthFixed, available_Width - label_width);
	}
	return table_bool;
}

inline void BeginWidget(const std::string& label)
{
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::Text(label.c_str());
	ImGui::TableNextColumn();
	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
}


void widgets::float_widget(const std::string& label,float* _value, float _step, float _min, float _max)
{
	BeginWidget(label);
	ImGui::DragFloat(("##"+label).c_str(),_value, _step, _min, _max);
}

void widgets::float2_widget(const std::string& label, float* _value, float _step, float _min, float _max)
{
	BeginWidget(label);
	ImGui::DragFloat2(("##"+label).c_str(),_value, _step, _min, _max);
}

void widgets::float3_widget(const std::string& label, float* _value, float _step, float _min, float _max)
{
	BeginWidget(label);
	ImGui::DragFloat3(("##"+label).c_str(),_value, _step, _min, _max);
}

void widgets::float4_widget(const std::string& label, float* _value, float _step, float _min, float _max)
{
	BeginWidget(label);
	ImGui::DragFloat4(("##"+label).c_str(),_value, _step, _min, _max);
}

void widgets::endWidgets()
{
    ImGui::EndTable();
}
