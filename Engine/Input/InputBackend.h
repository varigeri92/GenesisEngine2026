#pragma once
#include <map>
#include "api.h"
#include "glm/glm.hpp"

union SDL_Event;
struct SDL_Window;

namespace gns::core
{
	struct FrameInput
	{
		std::map<int, bool> keysDown;
		std::map<int, bool> keysHeld;
		std::map<int, bool> keysUp;

		std::map<int, bool> mouseDown;
		std::map<int, bool> mouseHeld;
		std::map<int, bool> mouseUp;

	};
	class InputBackend
	{
	public:

		static GNS_API bool GetKey(int keyCode);
		static GNS_API bool GetKeyUp(int keyCode);
		static GNS_API bool GetKeyDown(int keyCode);
		static GNS_API bool ProcessInput(SDL_Event& event, SDL_Window* window);
		static GNS_API bool GetMouseButtonDown(int mouseButton);
		static GNS_API bool GetMouseButton(int mouseButton);
		static GNS_API bool GetMouseButtonUp(int mouseButton);
		static GNS_API glm::vec2 GetMouseVelocity();
	private:
		void ProcessKeyboard();
		static FrameInput frameInput;
		static FrameInput previousFrameInput;
		static glm::vec2 mousePos;
		static glm::vec2 p_mousePos;
		static glm::vec2 mouseVelocity;
	};
}
