#include "Window.h"

#if defined(RENDERER_OPENGL)
#include "Rendering/OpenGL/Window_OpenGL.h"
#elif defined(RENDERER_VULKAN)
#include "Rendering/Vulkan/Window_Vulkan.h"
#elif defined(RENDERER_DX12)
#endif

Window* WindowFactory::Create()
{
#if defined(RENDERER_OPENGL)
	return new Window_OpenGL();
#elif defined(RENDERER_VULKAN)
	return new Window_Vulkan();
#elif defined(RENDERER_DX12)
#endif
	return nullptr;
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