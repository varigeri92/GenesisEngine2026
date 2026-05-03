#include "gnspch.h"
#include "Window.h"
#include "SDL2/SDL.h"
#include "../Input/InputBackend.h"

#include <algorithm>

namespace
{
    constexpr int ResizeBorderSize = 10;
    constexpr int TitleBarHeight = 28;
    constexpr int TitleBarButtonAreaWidth = 140;
    constexpr int MinWindowWidth = 320;
    constexpr int MinWindowHeight = 200;

    struct WindowResizeState
    {
        bool active = false;
        bool left = false;
        bool right = false;
        bool top = false;
        bool bottom = false;
        int startMouseX = 0;
        int startMouseY = 0;
        int startWindowX = 0;
        int startWindowY = 0;
        int startWindowWidth = 0;
        int startWindowHeight = 0;
    };

    WindowResizeState g_resizeState;
    SDL_Cursor* g_arrowCursor = nullptr;
    SDL_Cursor* g_sizeWECursor = nullptr;
    SDL_Cursor* g_sizeNSCursor = nullptr;
    SDL_Cursor* g_sizeNWSECursor = nullptr;
    SDL_Cursor* g_sizeNESWCursor = nullptr;

    void EnsureResizeCursors()
    {
        if (g_arrowCursor == nullptr) g_arrowCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
        if (g_sizeWECursor == nullptr) g_sizeWECursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
        if (g_sizeNSCursor == nullptr) g_sizeNSCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
        if (g_sizeNWSECursor == nullptr) g_sizeNWSECursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
        if (g_sizeNESWCursor == nullptr) g_sizeNESWCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
    }

    void GetResizeEdges(SDL_Window* window, int mouseX, int mouseY, bool& left, bool& right, bool& top, bool& bottom)
    {
        int width = 0;
        int height = 0;
        SDL_GetWindowSize(window, &width, &height);

        if (mouseY < TitleBarHeight && mouseX >= width - TitleBarButtonAreaWidth)
        {
            left = false;
            right = false;
            top = false;
            bottom = false;
            return;
        }

        left = mouseX < ResizeBorderSize;
        right = mouseX >= width - ResizeBorderSize;
        top = mouseY < ResizeBorderSize;
        bottom = mouseY >= height - ResizeBorderSize;
    }

    void UpdateResizeCursor(SDL_Window* window, int mouseX, int mouseY)
    {
        EnsureResizeCursors();

        bool left = false;
        bool right = false;
        bool top = false;
        bool bottom = false;
        GetResizeEdges(window, mouseX, mouseY, left, right, top, bottom);

        if ((left && top) || (right && bottom))
        {
            SDL_SetCursor(g_sizeNWSECursor);
        }
        else if ((right && top) || (left && bottom))
        {
            SDL_SetCursor(g_sizeNESWCursor);
        }
        else if (left || right)
        {
            SDL_SetCursor(g_sizeWECursor);
        }
        else if (top || bottom)
        {
            SDL_SetCursor(g_sizeNSCursor);
        }
        else
        {
            SDL_SetCursor(g_arrowCursor);
        }
    }

    bool BeginWindowResize(SDL_Window* window, const SDL_MouseButtonEvent& event)
    {
        if (event.button != SDL_BUTTON_LEFT)
        {
            return false;
        }

        GetResizeEdges(window, event.x, event.y, g_resizeState.left, g_resizeState.right, g_resizeState.top, g_resizeState.bottom);
        if (!g_resizeState.left && !g_resizeState.right && !g_resizeState.top && !g_resizeState.bottom)
        {
            return false;
        }

        g_resizeState.active = true;
        SDL_GetGlobalMouseState(&g_resizeState.startMouseX, &g_resizeState.startMouseY);
        SDL_GetWindowPosition(window, &g_resizeState.startWindowX, &g_resizeState.startWindowY);
        SDL_GetWindowSize(window, &g_resizeState.startWindowWidth, &g_resizeState.startWindowHeight);
        SDL_CaptureMouse(SDL_TRUE);
        return true;
    }

    void UpdateWindowResize(SDL_Window* window)
    {
        if (!g_resizeState.active)
        {
            return;
        }

        int mouseX = 0;
        int mouseY = 0;
        SDL_GetGlobalMouseState(&mouseX, &mouseY);

        const int deltaX = mouseX - g_resizeState.startMouseX;
        const int deltaY = mouseY - g_resizeState.startMouseY;

        int newX = g_resizeState.startWindowX;
        int newY = g_resizeState.startWindowY;
        int newWidth = g_resizeState.startWindowWidth;
        int newHeight = g_resizeState.startWindowHeight;

        if (g_resizeState.left)
        {
            newWidth = g_resizeState.startWindowWidth - deltaX;
            newX = g_resizeState.startWindowX + deltaX;
            if (newWidth < MinWindowWidth)
            {
                newWidth = MinWindowWidth;
                newX = g_resizeState.startWindowX + g_resizeState.startWindowWidth - MinWindowWidth;
            }
        }
        else if (g_resizeState.right)
        {
            newWidth = std::max(MinWindowWidth, g_resizeState.startWindowWidth + deltaX);
        }

        if (g_resizeState.top)
        {
            newHeight = g_resizeState.startWindowHeight - deltaY;
            newY = g_resizeState.startWindowY + deltaY;
            if (newHeight < MinWindowHeight)
            {
                newHeight = MinWindowHeight;
                newY = g_resizeState.startWindowY + g_resizeState.startWindowHeight - MinWindowHeight;
            }
        }
        else if (g_resizeState.bottom)
        {
            newHeight = std::max(MinWindowHeight, g_resizeState.startWindowHeight + deltaY);
        }

        SDL_SetWindowPosition(window, newX, newY);
        SDL_SetWindowSize(window, newWidth, newHeight);
    }

    void EndWindowResize()
    {
        if (!g_resizeState.active)
        {
            return;
        }

        g_resizeState.active = false;
        SDL_CaptureMouse(SDL_FALSE);
    }

    SDL_HitTestResult SDLCALL BorderlessWindowHitTest(SDL_Window* window, const SDL_Point* area, void* data)
    {
        (void)data;

        int width = 0;
        int height = 0;
        SDL_GetWindowSize(window, &width, &height);

        const bool left = area->x < ResizeBorderSize;
        const bool right = area->x >= width - ResizeBorderSize;
        const bool top = area->y < ResizeBorderSize;
        const bool bottom = area->y >= height - ResizeBorderSize;

        if (left || right || top || bottom)
        {
            return SDL_HITTEST_NORMAL;
        }

        const bool titleBarButtonArea = area->x >= width - TitleBarButtonAreaWidth;
        if (area->y < TitleBarHeight && !titleBarButtonArea)
        {
            return SDL_HITTEST_DRAGGABLE;
        }

        return SDL_HITTEST_NORMAL;
    }
}

bool gns::window::HandleBorderlessWindowEvent(SDL_Window* window, const SDL_Event& event, Uint32 mainWindowId)
{
    if (window == nullptr)
    {
        return false;
    }

    switch (event.type)
    {
    case SDL_MOUSEBUTTONDOWN:
        if (event.button.windowID == mainWindowId)
        {
            return BeginWindowResize(window, event.button);
        }
        break;
    case SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_LEFT)
        {
            EndWindowResize();
        }
        break;
    case SDL_MOUSEMOTION:
        if (g_resizeState.active)
        {
            UpdateWindowResize(window);
            return true;
        }
        if (event.motion.windowID == mainWindowId)
        {
            UpdateResizeCursor(window, event.motion.x, event.motion.y);
        }
        break;
    default:
        break;
    }

    return false;
}

gns::window::Window::Window(uint32_t width, uint32_t height, std::string title)
    : m_width(width), m_height(height), m_title(title), m_screen(width, height)
{
}

void gns::window::Window::Create()
{
    close = true;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        LOG_ERROR("Sdl window init failed!");
        return;
    }

    SDL_WindowFlags flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS);

    sdl_window = SDL_CreateWindow(
        m_title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        m_width, m_height, flags);

    if (!sdl_window) {
        LOG_ERROR("SDL Window Create Error!");
        SDL_Quit();
        return;
    }

    if (SDL_SetWindowHitTest(sdl_window, BorderlessWindowHitTest, nullptr) != 0)
    {
        LOG_ERROR("SDL window hit test setup failed!");
    }

    close = false;
    RefreshScreen();
}

void gns::window::Window::PollWindowEvents()
{
    SDL_Event e;
    close = gns::core::InputBackend::ProcessInput(e, sdl_window);
    RefreshScreen();
}

void gns::window::Window::CloseWindow()
{
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
}

const gns::Screen& gns::window::Window::GetScreen() const
{
    return m_screen;
}

void gns::window::Window::RefreshScreen()
{
    if (sdl_window == nullptr)
    {
        return;
    }

    int width = 0;
    int height = 0;
    SDL_GetWindowSize(sdl_window, &width, &height);
    m_screen.SetSize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}
