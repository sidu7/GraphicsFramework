#include "Inputs.h"
#include "Engine.h"

extern Engine* engine;

void Inputs::Init()
{
}

void Inputs::Update()
{
	SDL_Event e;
	while (SDL_PollEvent(&e) != 0)
	{
		if (e.type == SDL_QUIT)
		{
			engine->appIsRunning = false;
		}
	}
}

void Inputs::Close()
{
}
