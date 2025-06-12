#pragma once

#include "Core/Core.h"

class Texture;

class ComputeShader
{
public:
	virtual ~ComputeShader() {}

	virtual void SetShader(std::string path) = 0;
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
