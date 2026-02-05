#pragma once
#include <cstdint>
#include <string>
#include <SDL2/SDL.h>
namespace gns::window {

	class Window
	{
	public:
		Window(uint32_t width, uint32_t height, std::string title);
		~Window() = default;

		void Create();
		void PollWindowEvents();
		void CloseWindow();
		bool close;
		SDL_Window* sdl_window;
	private:
		uint32_t m_width;
		uint32_t m_height;
		std::string m_title;

	};
}