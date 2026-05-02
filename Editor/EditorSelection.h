#pragma once

#include "Genesis.h"

class EditorSelection
{
public:
    static void SelectEntity(gns::entityHandle entity);
    static gns::entityHandle GetSelectedEntity();
    static bool IsSelected(gns::entityHandle entity);

private:
    static gns::entityHandle SelectedEntity;
};
