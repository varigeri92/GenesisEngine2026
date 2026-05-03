#pragma once
#include <cstdint>
#include <string>
#include <SDL2/SDL.h>
#include "../Core/Screen.h"
namespace gns::window {
	bool HandleBorderlessWindowEvent(SDL_Window* window, const SDL_Event& event, Uint32 mainWindowId);

	class Window
	{
	public:
		Window(uint32_t width, uint32_t height, std::string title);
		~Window() = default;

		void Create();
		void PollWindowEvents();
		void CloseWindow();
		const gns::Screen& GetScreen() const;
		bool close;
		SDL_Window* sdl_window;
	private:
		void RefreshScreen();

		uint32_t m_width;
		uint32_t m_height;
		std::string m_title;
		gns::Screen m_screen;

	};
}
