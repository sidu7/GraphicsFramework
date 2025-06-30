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

struct ShaderSource
{
	std::vector<char> vertexSource;
	std::vector<char> fragmentSource;
};

class Shader
{
public:
	virtual ~Shader() {}

	// Must call all relevant of these calls before Init
	virtual void Uses(const FrameBuffer* framebuffer) = 0;
	virtual void Uses(const Texture* texture, unsigned int slot = 0) = 0;
	virtual void Uses(const UniformBuffer* uniformBuffer, unsigned int binding = 0) = 0;
	virtual void Uses(const VertexBuffer* vertexBuffer) = 0;
	virtual void Uses(const IndexBuffer* indexBuffer) = 0;

	virtual void Init(std::string vertexShaderId, std::string fragmentShaderId) = 0;
};