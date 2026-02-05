#include "gnspch.h"
#include "Window.h"
#include "SDL2/SDL.h"
#include "../Input/InputBackend.h"

gns::window::Window::Window(uint32_t width, uint32_t height, std::string title) : m_width(width), m_height(height), m_title(title)
{
}

void gns::window::Window::Create()
{
    close = true;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        LOG_ERROR("Sdl window init failed!");
        return;
    }

    SDL_WindowFlags flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

    sdl_window = SDL_CreateWindow(
        m_title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        m_width, m_height, flags);

    if (!sdl_window) {
        LOG_ERROR("SDL Window Create Error!");
        SDL_Quit();
        return;
    }
    close = false;
}

void gns::window::Window::PollWindowEvents()
{
    SDL_Event e;
    close = gns::core::InputBackend::ProcessInput(e);
}

void gns::window::Window::CloseWindow()
{
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
}
