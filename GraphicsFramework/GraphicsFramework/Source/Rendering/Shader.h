/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

#include "Core/Core.h"

class Texture;
class UniformBuffer;
class VertexBuffer;
class IndexBuffer;
class FrameBuffer;

// Shader base class.
// Applications register shaders with an int32_t ID (typically from a local enum)
// and pass that ID to Init(). The registry resolves paths for both OpenGL and Vulkan.
class Shader
{
public:
	virtual ~Shader() {}

	// Initialize from ShaderRegistry by ID (preferred — enum-based in application)
	virtual void Init(int32_t shaderId) = 0;

	// Initialize from raw file paths (legacy/direct path, for projects that bypass registry)
	virtual void Init(const std::string& vertexPath, const std::string& fragmentPath) = 0;
};
