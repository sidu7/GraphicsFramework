/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#include "ComputeShaderGL.h"

#include "Core/Core.h"
#include "RHI/OpenGL/DeviceGL.h"
#include "Utils/FileHelper.h"
#include "RHI/OpenGL/TextureGL.h"

namespace rhi::gl
{
ComputeShaderGL::ComputeShaderGL()
	: m_RendererID(0)
{
	GLCall(m_RendererID = glCreateProgram());
}

ComputeShaderGL::~ComputeShaderGL()
{
	GLCall(glDeleteProgram(m_RendererID));
}

unsigned int ComputeShaderGL::CompileShader(unsigned int type, const std::vector<char>& Source)
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
		std::cout << "Failed to compile compute shader" << std::endl;
		std::cout << message << std::endl;
		glDeleteShader(id);
		return 0;
	}

	return id;
}

void ComputeShaderGL::Init(int32_t shaderId)
{
	const auto& path = ShaderRegistry::Instance()->GetGLSLComputePath(shaderId);
	std::vector<char> shaderSource = FileHelper::ReadFile(path);

	if (shaderSource.empty())
	{
		std::cout << "Failed to load compute shader source for ID: " << shaderId << std::endl;
		return;
	}

	unsigned int shader = CompileShader(GL_COMPUTE_SHADER, shaderSource);
	if (shader == 0)
	{
		return;
	}

	GLCall(glAttachShader(m_RendererID, shader));
	GLCall(glLinkProgram(m_RendererID));

	int result;
	GLCall(glGetProgramiv(m_RendererID, GL_LINK_STATUS, &result));
	if (result == GL_FALSE)
	{
		int length;
		GLCall(glGetProgramiv(m_RendererID, GL_INFO_LOG_LENGTH, &length));
		char* message = (char*)alloca(length * sizeof(char));
		GLCall(glGetProgramInfoLog(m_RendererID, length, &length, message));
		std::cout << "Failed to link compute shader: " << path << std::endl;
		std::cout << message << std::endl;
	}
	else
	{
		GLCall(glValidateProgram(m_RendererID));
	}

	GLCall(glDeleteShader(shader));
}

void ComputeShaderGL::Init(const std::string& filePath)
{
	std::vector<char> shaderSource = FileHelper::ReadFile(filePath);

	if (shaderSource.empty())
	{
		std::cout << "Failed to load compute shader from path: " << filePath << std::endl;
		return;
	}

	unsigned int shader = CompileShader(GL_COMPUTE_SHADER, shaderSource);
	if (shader == 0)
	{
		return;
	}

	GLCall(glAttachShader(m_RendererID, shader));
	GLCall(glLinkProgram(m_RendererID));

	int result;
	GLCall(glGetProgramiv(m_RendererID, GL_LINK_STATUS, &result));
	if (result == GL_FALSE)
	{
		int length;
		GLCall(glGetProgramiv(m_RendererID, GL_INFO_LOG_LENGTH, &length));
		char* message = (char*)alloca(length * sizeof(char));
		GLCall(glGetProgramInfoLog(m_RendererID, length, &length, message));
		std::cout << "Failed to link compute shader" << std::endl;
		std::cout << message << std::endl;
	}
	else
	{
		GLCall(glValidateProgram(m_RendererID));
	}

	GLCall(glDeleteShader(shader));
}

void ComputeShaderGL::Run(unsigned int groupX, unsigned int groupY, unsigned int groupZ) const
{
	GLCall(glUseProgram(m_RendererID));
	GLCall(glDispatchCompute(groupX, groupY, groupZ));
	GLCall(glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_COMMAND_BARRIER_BIT));
}

void ComputeShaderGL::SetInputUniformImage(std::string name, const Texture* texture, unsigned int imageUnit, unsigned int channels) const
{
	unsigned int channelFlag;
	switch (channels)
	{
	case 1: channelFlag = GL_R32F; break;
	case 2: channelFlag = GL_RG32F; break;
	case 3: channelFlag = GL_RGB32F; break;
	case 4: channelFlag = GL_RGBA32F; break;
	}
	GLCall(unsigned int loc = glGetUniformLocation(m_RendererID, name.c_str()));
	GLCall(glBindImageTexture(imageUnit, static_cast<const TextureGL*>(texture)->mRendererID, 0, GL_FALSE, 0, GL_READ_ONLY, channelFlag));
	GLCall(glUniform1i(loc, imageUnit));
}

void ComputeShaderGL::SetOutputUniformImage(std::string name, const Texture* texture, unsigned int imageUnit, unsigned int channels) const
{
	unsigned int channelFlag;
	switch (channels)
	{
	case 1: channelFlag = GL_R32F; break;
	case 2: channelFlag = GL_RG32F; break;
	case 3: channelFlag = GL_RGB32F; break;
	case 4: channelFlag = GL_RGBA32F; break;
	}
	GLCall(unsigned int loc = glGetUniformLocation(m_RendererID, name.c_str()));
	GLCall(glBindImageTexture(imageUnit, static_cast<const TextureGL*>(texture)->mRendererID, 0, GL_FALSE, 0, GL_WRITE_ONLY, channelFlag));
	GLCall(glUniform1i(loc, imageUnit));
}

void ComputeShaderGL::SetUniformBlock(std::string name, unsigned int bindPoint)
{
	GLCall(unsigned int loc = glGetUniformBlockIndex(m_RendererID, name.c_str()));
	GLCall(glUniformBlockBinding(m_RendererID, loc, bindPoint));
}

void ComputeShaderGL::SetUniform1i(const std::string& name, int i)
{
	GLCall(glUniform1i(GetUniformLocation(name), i));
}

void ComputeShaderGL::SetUniform1f(const std::string& name, float v)
{
	GLCall(glUniform1f(GetUniformLocation(name), v));
}

void ComputeShaderGL::SetUniform2f(const std::string& name, float v1, float v2)
{
	GLCall(glUniform2f(GetUniformLocation(name), v1, v2));
}

void ComputeShaderGL::SetUniform2f(const std::string& name, glm::vec2 v)
{
	GLCall(glUniform2f(GetUniformLocation(name), v.x, v.y));
}

void ComputeShaderGL::SetUniform3f(const std::string& name, float v1, float v2, float v3)
{
	GLCall(glUniform3f(GetUniformLocation(name), v1, v2, v3));
}

void ComputeShaderGL::SetUniform3f(const std::string& name, glm::vec3 v)
{
	GLCall(glUniform3f(GetUniformLocation(name), v.x, v.y, v.z));
}

int ComputeShaderGL::GetUniformLocation(const std::string& name)
{
	GLCall(int location = glGetUniformLocation(m_RendererID, name.c_str()));
	return location;
}
}
