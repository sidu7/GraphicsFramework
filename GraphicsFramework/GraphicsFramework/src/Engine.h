#pragma once

#include <SDL.h>
#include <glm/glm.hpp>

class Camera;

class Engine
{
public:
	void Start();
	void Run();
	void Stop();

	glm::vec2 GetWindowSize();

	bool appIsRunning;
	bool stopMoving;

	SDL_Window* pWindow;
	SDL_GLContext glContext;
	int scrWidth, scrHeight;

	Camera* pCamera;
};