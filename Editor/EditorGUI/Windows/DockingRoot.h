#pragma once
#include <string>

#include "GenesisGUI.h"

class DockingRoot : public GuiWindow
{
public:
    explicit DockingRoot(std::string title);
    ~DockingRoot() override;
    void BeginWindow() override;
    void OnDraw() override;
    void EndWindow() override;
};
