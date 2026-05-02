#include "gnspch.h"
#include "Window.h"
#include "SDL2/SDL.h"
#include "../Input/InputBackend.h"

namespace
{
    constexpr int ResizeBorderSize = 10;
    constexpr int TitleBarDragHeight = 28;
    constexpr int TitleBarButtonAreaWidth = 140;

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
        if (area->y < TitleBarDragHeight && !titleBarButtonArea)
        {
            return SDL_HITTEST_DRAGGABLE;
        }

        return SDL_HITTEST_NORMAL;
    }
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
