/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#include "Shader.h"
#include "Renderer.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include "glm/gtc/type_ptr.hpp"

Shader::Shader(std::string vertexFilePath, std::string fragmentFilePath) 
	: m_VertexFilePath(vertexFilePath),m_FragmentFilePath(fragmentFilePath)
{
	ShaderSource shaders = ParseShader(vertexFilePath, fragmentFilePath);
	m_RendererID = CreateProgram(shaders.vertexSource, shaders.fragmentSource);
}

Shader::~Shader()
{
	GLCall(glDeleteProgram(m_RendererID));
}

ShaderSource Shader::ParseShader(std::string vertexFilePath, std::string fragmentFilePath)
{
	std::ifstream vertexFile(vertexFilePath);
	std::ifstream fragmentFile(fragmentFilePath);

	//TODO: Check whether shader file has actually opened
	std::stringstream ss[2];

	ss[0] << vertexFile.rdbuf();
	ss[1] << fragmentFile.rdbuf();

	vertexFile.close();
	fragmentFile.close();

	return { ss[0].str(), ss[1].str() };
}

unsigned int Shader::CompileShader(unsigned int type, std::string& Source)
{
	GLCall(unsigned int id = glCreateShader(type));
	const char* src = Source.c_str();
	GLCall(glShaderSource(id, 1, &src, nullptr));
	GLCall(glCompileShader(id));

	int result;
	GLCall(glGetShaderiv(id, GL_COMPILE_STATUS, &result));
	if (result == GL_FALSE)
	{
		int length;
		GLCall(glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length));
		char* message = (char*)alloca(length * sizeof(char));
		GLCall(glGetShaderInfoLog(id, length, &length, message));
		std::cout << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex " : "fragment ") << "shader" << std::endl;
		std::cout << message << std::endl;
		return 0;
	}

	return id;
}

unsigned int Shader::CreateProgram(std::string& vertexSource, std::string& fragmentSource)
{
	GLCall(unsigned int program = glCreateProgram());
	unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexSource);
	unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

	GLCall(glAttachShader(program, vs));
	GLCall(glAttachShader(program, fs));
	GLCall(glLinkProgram(program));

	int result;
	GLCall(glGetProgramiv(program, GL_LINK_STATUS, &result));
	if (result == GL_FALSE)
	{
		int length;
		GLCall(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length));
		char* message = (char*)alloca(length * sizeof(char));
		GLCall(glGetProgramInfoLog(program, length, &length, message));
		std::cout << "Failed to link " << std::endl;
		std::cout << message << std::endl;
		return 0;
	}

	GLCall(glValidateProgram(program));

	GLCall(glDeleteShader(vs));
	GLCall(glDeleteShader(fs));

	return program;
}

void Shader::Bind() const
{
	GLCall(glUseProgram(m_RendererID));
}
void Shader::Unbind() const
{
	GLCall(glUseProgram(0));
}

void Shader::SetUniform3f(const std::string& name, float v1, float v2, float v3)
{
	GLCall(glUniform3f(GetUniformLocation(name), v1, v2, v3));
}

void Shader::SetUniform3f(const std::string& name, glm::vec3 v)
{
	GLCall(glUniform3f(GetUniformLocation(name), v.x, v.y, v.z));
}

void Shader::SetUniform2f(const std::string& name, float v1, float v2)
{
	GLCall(glUniform2f(GetUniformLocation(name), v1, v2));
}

void Shader::SetUniform2f(const std::string& name, glm::vec2 v)
{
	GLCall(glUniform2f(GetUniformLocation(name), v.x, v.y));
}

void Shader::SetUniform4f(const std::string& name, float v1, float v2, float v3, float v4)
{
	GLCall(glUniform4f(GetUniformLocation(name), v1, v2, v3, v4));
}

void Shader::SetUniform4f(const std::string& name, glm::vec4 v)
{
	GLCall(glUniform4f(GetUniformLocation(name), v.x, v.y, v.z, v.w));
}

int Shader::GetUniformLocation(const std::string& name)
{
	GLCall(int location = glGetUniformLocation(m_RendererID, name.c_str()));

	//if (location == -1)
		//std::cout << "Warning: Uniform '" << name << "' doesn't exist" << std::endl;

	return location;
}

void Shader::SetUniform1i(const std::string& name, int i)
{
	GLCall(glUniform1i(GetUniformLocation(name), i));
}

void Shader::SetUniform1f(const std::string& name, float v)
{
	GLCall(glUniform1f(GetUniformLocation(name), v));
}

void Shader::SetUniformMat4f(const std::string& name, glm::mat4 matrix)
{
	GLCall(glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix)));
}

void Shader::SetUniformMat4fLoc(int location, glm::mat4 matrix)
{
	GLCall(glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix)));
}

void Shader::SetUniform2fvLoc(int location, glm::vec2 vector)
{
	GLCall(glUniform2fv(location, 1, glm::value_ptr(vector)));
}

void Shader::SetUniform3fvLoc(int location, glm::vec3 vector)
{
	GLCall(glUniform4fv(location, 1, glm::value_ptr(vector)));
}

void Shader::SetUniform4fvLoc(int location, glm::vec4 vector)
{
	GLCall(glUniform4fv(location, 1, glm::value_ptr(vector)));
}

void Shader::SetUniformBlock(const std::string& name, unsigned int bindPoint)
{
	GLCall(unsigned int loc = glGetUniformBlockIndex(m_RendererID, name.c_str()));
	GLCall(glUniformBlockBinding(m_RendererID, loc, bindPoint));
}
