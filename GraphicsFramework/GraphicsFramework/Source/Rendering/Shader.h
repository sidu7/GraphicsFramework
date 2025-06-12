/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

#include <string>
#include "glm/glm.hpp"

struct ShaderSource
{
	std::string vertexSource;
	std::string fragmentSource;
};

class Shader
{
public:
	virtual ~Shader() {}

	virtual unsigned int CreateProgram(std::string& vertexSource, std::string& fragmentSource) = 0;
	virtual unsigned int CompileShader(unsigned int type, std::string& Source) = 0;

	virtual void Init(std::string vertexFilePath, std::string fragmentFilePath) = 0;

	virtual void SetUniform1i(const std::string& name, int i) = 0;
	virtual void SetUniform1f(const std::string& name, float v) = 0;
	virtual void SetUniform2f(const std::string& name, float v1, float v2) = 0;
	virtual void SetUniform2f(const std::string& name, glm::vec2 v) = 0;
	virtual void SetUniform3f(const std::string& name, float v1, float v2, float v3) = 0;
	virtual void SetUniform3f(const std::string& name, glm::vec3 v) = 0;
	virtual void SetUniform4f(const std::string& name, float v1, float v2, float v3, float v4) = 0;
	virtual void SetUniform4f(const std::string& name, glm::vec4 v) = 0;
	virtual void SetUniformMat4f(const std::string& name, glm::mat4 matrix) = 0;
	virtual void SetUniformMat4fLoc(int location, glm::mat4 matrix) = 0;
	virtual int GetUniformLocation(const std::string& name) = 0;
	virtual void SetUniform2fvLoc(int location, glm::vec2 vector) = 0;
	virtual void SetUniform3fvLoc(int location, glm::vec3 vector) = 0;
	virtual void SetUniform4fvLoc(int location, glm::vec4 vector) = 0;
	virtual void SetUniformBlock(const std::string& name, unsigned int bindPoint) = 0;
};