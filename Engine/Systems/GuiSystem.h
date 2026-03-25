#pragma once
#include "Genesis.h"
#include "../Gui/GuiBackend.h"
#include "../Gui/GuiWindow.h"

class GuiSystem : public gns::core::System
{
    gns::gui::GuiBackend gui_backend = {};
    std::vector<std::unique_ptr<GuiWindow>> Windows = {};
public:
    template 
    <typename window_T, typename = std::enable_if<std::is_base_of<GuiWindow, window_T>::value>::type, typename... Args>
    window_T* RegisterWindow(Args&& ... args)
    {
        Windows.emplace_back(std::make_unique<window_T>(std::forward<Args>(args)...));
        return dynamic_cast<window_T*>(Windows[Windows.size() - 1].get());
    }
    
    template 
    <typename window_T, typename = std::enable_if<std::is_base_of<GuiWindow, window_T>::value>::type, typename... Args>
    window_T* GetWindowByTitle(const std::string& title)
    {
        for (auto& window : Windows)
        {
            if (window->GetTitle() == title)
            {
                return dynamic_cast<window_T*>(window.get());
            }
        }
        return nullptr;
    }

    void DrawWindows() const;
    
    GuiSystem();
    void OnCreate() override;
    void OnStart() override;
    void OnEnable() override;
    void OnUpdate(float deltaTime) override;
    void OnLateUpdate(float deltaTime) override;
    void OnFixedUpdate() override;
    void OnDisable() override;
    void OnDestroy() override;
};
