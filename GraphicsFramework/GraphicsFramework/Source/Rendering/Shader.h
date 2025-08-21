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

	virtual void Init(std::string shaderId) = 0;
};