#include "Time.h"
#include "SDL/include/SDL.h"

#include <thread>
#include <chrono>

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
	if (neededTime > 0 && endTime - startTime < neededTime)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(neededTime - static_cast<unsigned int>(endTime - startTime)));
		endTime = SDL_GetTicks();
	}
	deltaTime = (endTime - startTime) / 1000.0f;
}

void Time::SetFrameRate(int frameRate)
{
	if (frameRate > 0)
	{
		neededTime = 1000 / frameRate;
	}
	else
	{
		neededTime = 0;
	}
}
