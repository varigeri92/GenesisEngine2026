#pragma once

class GuiWindow
{
protected:
    bool open = true;
    std::string Title = "new Window";
public:
    GuiWindow(std::string title);
    virtual ~GuiWindow() = default;
    std::string& GetTitle() {return Title;};
public:
    //virtual void OnCreate();
    //virtual void OnOpen();
    virtual void OnDraw() = 0;
    //virtual void OnClose();
    //virtual void OnDestroy();
};