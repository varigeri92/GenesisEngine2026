#include "gnspch.h"
#include "API.h"
#include "InputBackend.h"
#include "../Window/Window.h"
#include "../Gui/GuiBackend.h"

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
		const Uint32 mainWindowId = window != nullptr ? SDL_GetWindowID(window) : 0;

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
				gns::window::HandleBorderlessWindowEvent(window, event, mainWindowId);
				break;
			case SDL_MOUSEBUTTONUP:
				frameInput.mouseUp[event.button.button] = true;
				frameInput.mouseHeld[event.button.button] = false;
				gns::window::HandleBorderlessWindowEvent(window, event, mainWindowId);
				break;
			case SDL_MOUSEMOTION:
				mousePos.x = static_cast<float>(event.motion.x);
				mousePos.y = static_cast<float>(event.motion.y);
				mouseDelta.x += static_cast<float>(event.motion.xrel);
				mouseDelta.y += static_cast<float>(event.motion.yrel);
				gns::window::HandleBorderlessWindowEvent(window, event, mainWindowId);
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
