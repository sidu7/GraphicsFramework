/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#include "Shader_OpenGL.h"
#include "Rendering/OpenGL/Renderer_OpenGL.h"
#include "Utils/FileHelper.h"

Shader_OpenGL::Shader_OpenGL()
	: m_RendererID(0)
{
	GLCall(m_RendererID = glCreateProgram());
}

void Shader_OpenGL::Init(int32_t shaderId)
{
	const auto& vertPath = ShaderRegistry::Instance()->GetGLSLPath(shaderId, ShaderRegistry::ShaderStage::Vertex);
	const auto& fragPath = ShaderRegistry::Instance()->GetGLSLPath(shaderId, ShaderRegistry::ShaderStage::Fragment);

	auto vertSource = FileHelper::ReadFile(vertPath);
	auto fragSource = FileHelper::ReadFile(fragPath);

	if (vertSource.empty() || fragSource.empty())
	{
		std::cout << "Failed to load shader source for ID: " << shaderId << std::endl;
		return;
	}

	unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertSource);
	unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragSource);

	if (vs == 0 || fs == 0)
	{
		std::cout << "Shader compilation failed for ID: " << shaderId << std::endl;
		if (vs) glDeleteShader(vs);
		if (fs) glDeleteShader(fs);
		return;
	}

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
		std::cout << "Failed to link shader: " << vertPath << std::endl;
		std::cout << message << std::endl;
	}
	else
	{
		GLCall(glValidateProgram(m_RendererID));
	}

	GLCall(glDeleteShader(vs));
	GLCall(glDeleteShader(fs));
}

void Shader_OpenGL::Init(const std::string& vertexPath, const std::string& fragmentPath)
{
	auto vertSource = FileHelper::ReadFile(vertexPath);
	auto fragSource = FileHelper::ReadFile(fragmentPath);

	if (vertSource.empty() || fragSource.empty())
	{
		std::cout << "Failed to load shader source from paths: " << vertexPath << ", " << fragmentPath << std::endl;
		return;
	}

	unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertSource);
	unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragSource);

	if (vs == 0 || fs == 0)
	{
		std::cout << "Shader compilation failed for paths: " << vertexPath << ", " << fragmentPath << std::endl;
		if (vs) glDeleteShader(vs);
		if (fs) glDeleteShader(fs);
		return;
	}

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
		std::cout << "Failed to link shader: " << vertexPath << std::endl;
		std::cout << message << std::endl;
	}
	else
	{
		GLCall(glValidateProgram(m_RendererID));
	}

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
		GLCall(glGetShaderiv(id, GL_COMPILE_STATUS, &length));
		char* message = (char*)alloca(length * sizeof(char));
		GLCall(glGetShaderInfoLog(id, length, &length, message));
		std::cout << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex " : "fragment ") << "shader" << std::endl;
		std::cout << message << std::endl;
		glDeleteShader(id);
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
