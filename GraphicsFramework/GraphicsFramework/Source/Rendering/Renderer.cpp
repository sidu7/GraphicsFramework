#include "Renderer.h"

#include "Core/Core.h"
#if defined(RENDERER_OPENGL)
#include "Rendering/OpenGL/Renderer_OpenGL.h"
#elif defined(RENDERER_VULKAN)
#include "Rendering/Vulkan/Renderer_Vulkan.h"
#elif defined(RENDERER_DX12)
#endif

Renderer* RendererFactory::Create()
{
#if defined(RENDERER_OPENGL)
	return new Renderer_OpenGL();
#elif defined(RENDERER_VULKAN)
	return new Renderer_Vulkan();
#elif defined(RENDERER_DX12)
#endif
	return nullptr;
}