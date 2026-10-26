#include "RHI/Backend.h"

#include "RHI/Vulkan/DeviceVK.h"
#include "RHI/Vulkan/WindowVK.h"

namespace rhi
{
	Renderer* CreateDevice()
	{
		return new rhi::vk::Device();
	}

	Window* CreateWindow()
	{
		return new rhi::vk::WindowVK();
	}
}
