#pragma once

#include <SDL.h>

class Engine
{
public:
	void Start();
	void Run();
	void Stop();

	bool appIsRunning;

	SDL_Window* pWindow;	
};