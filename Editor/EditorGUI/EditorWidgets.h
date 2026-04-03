#pragma once
#include <string>

namespace widgets
{
    bool startWidgets(const std::string& widgetGroupID);
    void float_widget(const std::string& label, float* _value, float _step = 0.1f, float _min = 0, float _max = 0);
    void float2_widget(const std::string& label, float* _value, float _step = 0.1f, float _min = 0, float _max = 0);
    void float3_widget(const std::string& label, float* _value, float _step = 0.1f, float _min = 0, float _max = 0);
    void float4_widget(const std::string& label, float* _value, float _step = 0.1f, float _min = 0, float _max = 0);
    void endWidgets();
}
