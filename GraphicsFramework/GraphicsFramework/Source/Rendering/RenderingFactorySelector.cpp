#include "Rendering/RenderingFactory.h"

#if defined(RENDERER_OPENGL)
#include "Rendering/OpenGL/RenderingFactory_OpenGL.h"
#elif defined(RENDERER_VULKAN)
#include "Rendering/Vulkan/RenderingFactory_Vulkan.h"
#elif defined(RENDERER_DX12)
#endif

RenderingFactory* RenderingFactorySelector::Create()
{
#if defined(RENDERER_OPENGL)
	return new RenderingFactory_OpenGL();
#elif defined(RENDERER_VULKAN)
	return new RenderingFactory_Vulkan();
#elif defined(RENDERER_DX12)
#endif
}