#include "RHI/Backend.h"

#include "RHI/OpenGL/DeviceGL.h"
#include "RHI/OpenGL/WindowGL.h"

namespace rhi
{
	Renderer* CreateDevice()
	{
		return new rhi::gl::Device();
	}

	Window* CreateWindow()
	{
		return new rhi::gl::WindowGL();
	}
}
