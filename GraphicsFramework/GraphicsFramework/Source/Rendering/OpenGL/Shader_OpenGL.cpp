/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#include "Shader_OpenGL.h"
#include "Rendering/OpenGL/Renderer_OpenGL.h"
#include "Rendering/ShaderManager.h"
#include "Core/Core.h"
#include "Utils/FileHelper.h"

Shader_OpenGL::Shader_OpenGL()
	: m_RendererID(0)
{
	GLCall(m_RendererID = glCreateProgram());
}

void Shader_OpenGL::Init(std::string shaderId)
{
	ShaderSource shaders = ShaderManager::Instance()->GetShaderSource(shaderId);

	unsigned int vs = CompileShader(GL_VERTEX_SHADER, shaders.vertexSource);
	unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, shaders.fragmentSource);

	GLCall(glAttachShader(m_RendererID, vs));
	GLCall(glAttachShader(m_RendererID, fs));
	GLCall(glLinkProgram(m_RendererID));

	int result;
	GLCall(glGetProgramiv(m_RendererID, GL_LINK_STATUS, &result));
	if (result == GL_FALSE)
	{
		int length;
		GLCall(glGetProgramiv(m_RendererID, GL_INFO_LOG_LENGTH, &length));
		char* message = (char*)alloca(length * sizeof(char));
		GLCall(glGetProgramInfoLog(m_RendererID, length, &length, message));
		std::cout << "Failed to link " << std::endl;
		std::cout << message << std::endl;
		return;
	}

	GLCall(glValidateProgram(m_RendererID));

	GLCall(glDeleteShader(vs));
	GLCall(glDeleteShader(fs));
}

Shader_OpenGL::~Shader_OpenGL()
{
	GLCall(glDeleteProgram(m_RendererID));
}

unsigned int Shader_OpenGL::CompileShader(unsigned int type, const std::vector<char>& Source)
{
	GLCall(unsigned int id = glCreateShader(type));
	const char* src = Source.data();
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

void Shader_OpenGL::SetUniform3f(const std::string& name, float v1, float v2, float v3)
{
	GLCall(glUniform3f(GetUniformLocation(name), v1, v2, v3));
}

void Shader_OpenGL::SetUniform3f(const std::string& name, glm::vec3 v)
{
	GLCall(glUniform3f(GetUniformLocation(name), v.x, v.y, v.z));
}

void Shader_OpenGL::SetUniform2f(const std::string& name, float v1, float v2)
{
	GLCall(glUniform2f(GetUniformLocation(name), v1, v2));
}

void Shader_OpenGL::SetUniform2f(const std::string& name, glm::vec2 v)
{
	GLCall(glUniform2f(GetUniformLocation(name), v.x, v.y));
}

void Shader_OpenGL::SetUniform4f(const std::string& name, float v1, float v2, float v3, float v4)
{
	GLCall(glUniform4f(GetUniformLocation(name), v1, v2, v3, v4));
}

void Shader_OpenGL::SetUniform4f(const std::string& name, glm::vec4 v)
{
	GLCall(glUniform4f(GetUniformLocation(name), v.x, v.y, v.z, v.w));
}

int Shader_OpenGL::GetUniformLocation(const std::string& name)
{
	GLCall(int location = glGetUniformLocation(m_RendererID, name.c_str()));

	//if (location == -1)
		//std::cout << "Warning: Uniform '" << name << "' doesn't exist" << std::endl;

	return location;
}

void Shader_OpenGL::SetUniform1i(const std::string& name, int i)
{
	GLCall(glUniform1i(GetUniformLocation(name), i));
}

void Shader_OpenGL::SetUniform1f(const std::string& name, float v)
{
	GLCall(glUniform1f(GetUniformLocation(name), v));
}

void Shader_OpenGL::SetUniformMat4f(const std::string& name, glm::mat4 matrix)
{
	GLCall(glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix)));
}

void Shader_OpenGL::SetUniformMat4fLoc(int location, glm::mat4 matrix)
{
	GLCall(glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix)));
}

void Shader_OpenGL::SetUniform2fvLoc(int location, glm::vec2 vector)
{
	GLCall(glUniform2fv(location, 1, glm::value_ptr(vector)));
}

void Shader_OpenGL::SetUniform3fvLoc(int location, glm::vec3 vector)
{
	GLCall(glUniform4fv(location, 1, glm::value_ptr(vector)));
}

void Shader_OpenGL::SetUniform4fvLoc(int location, glm::vec4 vector)
{
	GLCall(glUniform4fv(location, 1, glm::value_ptr(vector)));
}

void Shader_OpenGL::SetUniformBlock(const std::string& name, unsigned int bindPoint)
{
	GLCall(unsigned int loc = glGetUniformBlockIndex(m_RendererID, name.c_str()));
	GLCall(glUniformBlockBinding(m_RendererID, loc, bindPoint));
}
