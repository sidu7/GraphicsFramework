#pragma once

#include <glm/glm.hpp>

// Backend-agnostic rendering types.
// These are shared by the abstract RHI interfaces and must not depend on
// any graphics API (OpenGL / Vulkan) or platform header.

enum class CullFace
{
	None,
	Front,
	Back,
	Both
};

struct Rect2D
{
	Rect2D() : Offset(0.f), Size(0.f) {}

	glm::vec2 Offset;
	glm::vec2 Size;
};

struct Rect3D : public Rect2D
{
	Rect3D() : Depth(0.0f, 1.0f) {}
	glm::vec2 Depth;
};
