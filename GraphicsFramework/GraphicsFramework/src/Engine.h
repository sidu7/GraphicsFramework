#pragma once

#include <SDL.h>
#include <glm/glm.hpp>

class Engine
{
public:
	void Start();
	void Run();
	void Stop();

	glm::vec2 GetWindowSize();

	bool appIsRunning;

	SDL_Window* pWindow;
};