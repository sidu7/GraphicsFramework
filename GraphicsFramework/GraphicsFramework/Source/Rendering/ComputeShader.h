/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

#include "Core/Core.h"

class Texture;

// ComputeShader base class.
// Applications register compute shaders with an int32_t ID (typically from a local enum)
// and pass that ID to SetPipeline(). The registry resolves paths for both OpenGL and Vulkan.
class ComputeShader
{
public:
	virtual ~ComputeShader() {}

	// Initialize from ShaderRegistry by ID (preferred — enum-based in application)
	virtual void Init(int32_t shaderId) = 0;

	// Initialize from raw file path (legacy/direct path)
	virtual void Init(const std::string& filePath) = 0;

	virtual void Run(unsigned int groupX, unsigned int groupY, unsigned int groupZ) const = 0;

	virtual void SetInputUniformImage(std::string name, const Texture* texture, unsigned int imageUnit, unsigned int channels) const = 0;
	virtual void SetOutputUniformImage(std::string name, const Texture* texture, unsigned int imageUnit, unsigned int channels) const = 0;
	virtual void SetUniformBlock(std::string name, unsigned int bindPoint) = 0;
	virtual void SetUniform1i(const std::string& name, int i) = 0;
	virtual void SetUniform1f(const std::string& name, float v) = 0;
	virtual void SetUniform2f(const std::string& name, float v1, float v2) = 0;
	virtual void SetUniform2f(const std::string& name, glm::vec2 v) = 0;
	virtual void SetUniform3f(const std::string& name, float v1, float v2, float v3) = 0;
	virtual void SetUniform3f(const std::string& name, glm::vec3 v) = 0;
	virtual int GetUniformLocation(const std::string& name) = 0;
};
