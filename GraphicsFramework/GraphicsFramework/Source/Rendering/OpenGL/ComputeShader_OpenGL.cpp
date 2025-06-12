#include "ComputeShader_OpenGL.h"

#include "Core/Core.h"
#include "Rendering/Renderer.h"
#include "Utils/FileHelper.h"
#include "Rendering/OpenGL/Texture_OpenGL.h"

ComputeShader_OpenGL::ComputeShader_OpenGL() 
	: m_RendererID(0)
{
	GLCall(m_RendererID = glCreateProgram());
}

ComputeShader_OpenGL::~ComputeShader_OpenGL()
{
}

unsigned int ComputeShader_OpenGL::CompileShader(unsigned int type, std::string& Source)
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

void ComputeShader_OpenGL::SetShader(std::string path)
{
	m_ShaderFilePath = path;
	std::string shaderSource = FileHelper::ReadFile(path);
	unsigned int shader = CompileShader(GL_COMPUTE_SHADER,shaderSource);

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
		std::cout << "Failed to link " << std::endl;
		std::cout << message << std::endl;
		return;
	}

	GLCall(glValidateProgram(m_RendererID));

	GLCall(glDeleteShader(shader));
}

void ComputeShader_OpenGL::Run(unsigned int groupX, unsigned int groupY, unsigned int groupZ) const
{
	GLCall(glDispatchCompute(groupX, groupY, groupZ));
}

void ComputeShader_OpenGL::SetInputUniformImage(std::string name, const Texture* texture, unsigned int imageUnit, unsigned int channels) const
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
	GLCall(glBindImageTexture(imageUnit, static_cast<const Texture_OpenGL*>(texture)->mRendererID, 0, GL_FALSE, 0, GL_READ_ONLY, channelFlag));
	GLCall(glUniform1i(loc, imageUnit));
}

void ComputeShader_OpenGL::SetOutputUniformImage(std::string name, const Texture* texture, unsigned int imageUnit, unsigned int channels) const
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
	GLCall(glBindImageTexture(imageUnit, static_cast<const Texture_OpenGL*>(texture)->mRendererID, 0, GL_FALSE, 0, GL_WRITE_ONLY, channelFlag));
	GLCall(glUniform1i(loc, imageUnit));
}

void ComputeShader_OpenGL::SetUniformBlock(std::string name, unsigned int bindPoint)
{
	GLCall(unsigned int loc = glGetUniformBlockIndex(m_RendererID, name.c_str()));
	GLCall(glUniformBlockBinding(m_RendererID, loc, bindPoint));
}

void ComputeShader_OpenGL::SetUniform1i(const std::string& name, int i)
{
	GLCall(glUniform1i(GetUniformLocation(name), i));
}

void ComputeShader_OpenGL::SetUniform1f(const std::string& name, float v)
{
	GLCall(glUniform1f(GetUniformLocation(name), v));
}

void ComputeShader_OpenGL::SetUniform2f(const std::string& name, float v1, float v2)
{
	GLCall(glUniform2f(GetUniformLocation(name), v1, v2));
}

void ComputeShader_OpenGL::SetUniform2f(const std::string& name, glm::vec2 v)
{
	GLCall(glUniform2f(GetUniformLocation(name), v.x, v.y));
}

void ComputeShader_OpenGL::SetUniform3f(const std::string& name, float v1, float v2, float v3)
{
	GLCall(glUniform3f(GetUniformLocation(name), v1, v2, v3));
}

void ComputeShader_OpenGL::SetUniform3f(const std::string& name, glm::vec3 v)
{
	GLCall(glUniform3f(GetUniformLocation(name), v.x, v.y, v.z));
}

int ComputeShader_OpenGL::GetUniformLocation(const std::string& name)
{
	GLCall(int location = glGetUniformLocation(m_RendererID, name.c_str()));
	return location;
}
