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

	//printf("%i joysticks were found.\n\n", SDL_NumJoysticks());//NUmber of joysticks
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// Enable AntiAliasiing
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

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

	glContext = nullptr;
	glContext = SDL_GL_CreateContext(pWindow);
	if (glContext == NULL)
	{
		printf("Vulkan context could not be created. SDL Error: %s\n", SDL_GetError());
	}
}

void Window_Vulkan::Close()
{
	SDL_DestroyWindow(pWindow);
	SDL_Quit();
}