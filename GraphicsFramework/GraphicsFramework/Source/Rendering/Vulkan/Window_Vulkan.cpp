#include "Window_Vulkan.h"
#include "Core/Core.h"
#include <SDL_vulkan.h>

void Window_Vulkan::Init(int width, int height)
{
	int error = 0;

	//Initialize SDL
	if ((error = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER)) < 0)//Init for video and joystick
	{
		printf("Couldn't initialize SDL, error %i\n", error);
		return;
	}
	SDL_Vulkan_LoadLibrary(nullptr);

	scrWidth = width;
	scrHeight = height;

	pWindow = SDL_CreateWindow("Graphics Framework",
							   SDL_WINDOWPOS_CENTERED,
							   SDL_WINDOWPOS_CENTERED,
							   scrWidth,
							   scrHeight,
							   SDL_WINDOW_VULKAN);

	//Check if window was made successfully
	if (pWindow == NULL)
	{
		printf("Could not create window: %s\n", SDL_GetError());
		return;
	}
}

void Window_Vulkan::Close()
{
	SDL_DestroyWindow(pWindow);
	SDL_Quit();
}