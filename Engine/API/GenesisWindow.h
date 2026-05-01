#pragma once
#include "API.h"

namespace gns::window
{
    GNS_API void MinimizeMainWindow();
    GNS_API void ToggleMaximizeMainWindow();
    GNS_API bool IsMainWindowMaximized();
    GNS_API void RequestCloseMainWindow();
}
