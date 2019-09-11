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
private:
	unsigned int m_RendererID;
	std::string m_VertexFilePath, m_FragmentFilePath;
public:
	Shader(std::string vertexFilePath, std::string fragmentFilePath);
	~Shader();

	inline unsigned int GetRenderId() { return m_RendererID; }

	unsigned int CreateProgram(std::string& vertexSource, std::string& fragmentSource);
	unsigned int CompileShader(unsigned int type, std::string& Source);
	ShaderSource ParseShader(std::string vertexFilePath, std::string fragmentFilePath);

	void Bind() const;
	void Unbind() const;
	void SetUniform1i(const std::string& name, int i);
	void SetUniform1f(const std::string& name, float v);
	void SetUniform2f(const std::string& name, float v1, float v2);
	void SetUniform3f(const std::string& name, float v1, float v2, float v3);
	void SetUniform4f(const std::string& name, float v1, float v2, float v3, float v4);
	void SetUniformMat4f(const std::string& name, glm::mat4 matrix);
	void SetUniformMat4fLoc(int location, glm::mat4 matrix);
	int GetUniformLocation(const std::string& name);
	void SetUniform2fvLoc(int location, glm::vec2 vector);
	void SetUniform3fvLoc(int location, glm::vec3 vector);
	void SetUniform4fvLoc(int location, glm::vec4 vector);
};