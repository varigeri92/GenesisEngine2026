#pragma once
#include <string_view>
#include <string>
#include "Genesis.h"
#include "GenesisGUI.h"


class SystemViewer : public GuiWindow
{
public:
    SystemViewer(const std::string& title);
    void BeginWindow() override;
    void OnDraw() override;
    void EndWindow() override;
private:

    struct SmoothedValue
    {
        double value = 0.0;
        bool hasSample = false;

        void AddSample(double sample, double smoothing = 0.10)
        {
            if (!hasSample)
            {
                value = sample;
                hasSample = true;
                return;
            }

            value = value * (1.0 - smoothing) + sample * smoothing;
        }
    };

    SmoothedValue m_frameTimeMs;
    SmoothedValue m_fps;
    
    static constexpr std::string_view SystemStateToString(gns::core::System::SystemState state)
    {
        using SystemState = gns::core::System::SystemState;

        switch (state)
        {
        case SystemState::Created:   return "Created";
        case SystemState::Started:   return "Started";
        case SystemState::Running:   return "Running";
        case SystemState::Stopped:   return "Stopped";
        case SystemState::Disabled:  return "Disabled";
        case SystemState::Destroyed: return "Destroyed";
        case SystemState::Enabled:   return "Enabled";
        default:                     return "Unknown";
        }
    }
};
