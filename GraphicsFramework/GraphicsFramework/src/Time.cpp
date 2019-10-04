#include "Time.h"
#include "SDL.h"

void Time::Init(unsigned int requiredFrameRate)
{
	deltaTime = 0;
	startTime = 0;
	endTime = 0;
	neededTime = 1000 / requiredFrameRate;
}

void Time::FrameStart()
{
	startTime = SDL_GetTicks();
}

void Time::FrameEnd()
{
	endTime = SDL_GetTicks();
	while (endTime - startTime < neededTime)
	{
		endTime = SDL_GetTicks();
	}
	deltaTime = (endTime - startTime) / 1000.0f;
}