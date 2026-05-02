#include "gnspch.h"
#include "API.h"
#include "InputBackend.h"
#include "../Window/Window.h"
#include "../Gui/GuiBackend.h"

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
}

namespace gns::core
{
	FrameInput InputBackend::frameInput = {};
	FrameInput InputBackend::previousFrameInput = {};
	glm::vec2 InputBackend::mousePos = {};
	glm::vec2 InputBackend::p_mousePos = {};
	glm::vec2 InputBackend::mouseDelta = {};


	bool InputBackend::GetKey(int keyCode)
	{
		return frameInput.keysHeld[keyCode];
	}
	bool InputBackend::GetKeyUp(int keyCode)
	{
		return frameInput.keysUp[keyCode];
	}

	bool InputBackend::GetKeyDown(int keyCode)
	{
		return frameInput.keysDown[keyCode];
	}

	bool InputBackend::ProcessInput(SDL_Event& event, SDL_Window* window)
	{
		frameInput.keysDown.clear();
		frameInput.keysUp.clear();

		frameInput.mouseDown.clear();
		frameInput.mouseUp.clear();

		p_mousePos.x = mousePos.x;
		p_mousePos.y = mousePos.y;
		mouseDelta = { 0,0 };

		while (SDL_PollEvent(&event) != 0)
		{
			switch (event.type)
			{
			case SDL_QUIT:
				return true;
			case SDL_KEYDOWN:
				if (!frameInput.keysDown[event.key.keysym.sym])
				{
					frameInput.keysDown[event.key.keysym.sym] = true;
				}
				frameInput.keysHeld[event.key.keysym.sym] = true;
				break;
			case SDL_KEYUP:
				frameInput.keysUp[event.key.keysym.sym] = true;
				frameInput.keysHeld[event.key.keysym.sym] = false;
				break;
			case SDL_MOUSEBUTTONDOWN:
				frameInput.mouseDown[event.button.button] = true;
				frameInput.mouseHeld[event.button.button] = true;
				BeginWindowResize(window, event.button);
				break;
			case SDL_MOUSEBUTTONUP:
				frameInput.mouseUp[event.button.button] = true;
				frameInput.mouseHeld[event.button.button] = false;
				if (event.button.button == SDL_BUTTON_LEFT)
				{
					EndWindowResize();
				}
				break;
			case SDL_MOUSEMOTION:
				mousePos.x = static_cast<float>(event.motion.x);
				mousePos.y = static_cast<float>(event.motion.y);
				mouseDelta.x += static_cast<float>(event.motion.xrel);
				mouseDelta.y += static_cast<float>(event.motion.yrel);
				if (g_resizeState.active)
				{
					UpdateWindowResize(window);
				}
				else
				{
					UpdateResizeCursor(window, event.motion.x, event.motion.y);
				}
				break;
			default:
				break;
			}
			gns::gui::GuiBackend::HandleEvents(event);
		}
		return false;
	}
	bool InputBackend::GetMouseButtonDown(int mouseButton)
	{
		return frameInput.mouseDown[mouseButton];
	}
	bool InputBackend::GetMouseButtonUp(int mouseButton)
	{
		return frameInput.mouseUp[mouseButton];
	}

	glm::vec2 InputBackend::GetMouseDelta()
	{
		return mouseDelta;
	}

	glm::vec2 InputBackend::GetMouseVelocity()
	{
		return GetMouseDelta();
	}

	bool InputBackend::GetMouseButton(int mouseButton)
	{
		return frameInput.mouseHeld[mouseButton];
	}
}
