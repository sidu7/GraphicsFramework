#include "Window.h"

#include "RHI/Backend.h"

Window* Window::Instance()
{
	static Window* instance = rhi::CreateWindow();
	return instance;
}

SDL_Window* Window::GetWindow() const
{
	return pWindow;
}

glm::vec2 Window::GetWindowSize()
{
	int x, y;
	SDL_GetWindowSize(pWindow, &x, &y);
	return glm::vec2(x, y);
}

int Window::GetWidth()
{
	return scrWidth;
}

int Window::GetHeight()
{
	return scrHeight;
}