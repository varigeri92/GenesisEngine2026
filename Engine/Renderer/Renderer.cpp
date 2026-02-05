#include "gnspch.h"
#include "Renderer.h"
#include "../Log/Logger.h"

void gns::rendering::Renderer::CreateDevice(SDL_Window* sdl_window)
{
	m_device.Create(sdl_window);
}
