#pragma once
#include "API.h"
class GuiWindow
{
protected:
    bool open = true;
    std::string Title = "new Window";
public:
    GNS_API GuiWindow(std::string title);
    GNS_API virtual ~GuiWindow() = default;
    GNS_API std::string& GetTitle() {return Title;}
    GNS_API virtual void BeginWindow();
    GNS_API virtual void OnDraw() = 0;
    GNS_API virtual void EndWindow();
};