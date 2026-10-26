#pragma once

class Renderer;
class Window;

// Backend seam.
//
// CMake compiles exactly one backend per configuration, so exactly one of
// RHI/OpenGL/BackendGL.cpp or RHI/Vulkan/BackendVK.cpp
// is linked. These are therefore the only definitions of the symbols below,
// and a configuration that links neither (or both) fails at link time instead
// of silently falling through a runtime #if chain.
//
// Phase 4 moves the definitions into RHI/OpenGL/BackendGL.cpp and
// RHI/Vulkan/BackendVK.cpp when the backend folders are restructured.
namespace rhi
{
	// Creates the rendering device (the concrete Renderer implementation).
	Renderer* CreateDevice();

	// Creates the window implementation belonging to the selected backend.
	Window* CreateWindow();
}
