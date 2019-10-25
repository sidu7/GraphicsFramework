#include "ComputeShader.h"
#include "Renderer.h"

#include <fstream>
#include <sstream>
#include <iostream>

ComputeShader::ComputeShader(std::string path) : m_RendererID(0), m_ShaderFilePath(path)
{
	std::string shaderSource = ParseShader(path);
	m_RendererID = CreateProgram(shaderSource);
}

ComputeShader::~ComputeShader()
{
}

unsigned int ComputeShader::CreateProgram(std::string& shaderSource)
{
	GLCall(unsigned int program = glCreateProgram());
	unsigned int cs = CompileShader(GL_COMPUTE_SHADER, shaderSource);

	GLCall(glAttachShader(program, cs));
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

	GLCall(glDeleteShader(cs));

	return program;
}

unsigned int ComputeShader::CompileShader(unsigned int type, std::string& Source)
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

std::string ComputeShader::ParseShader(std::string shaderFilePath)
{
	std::ifstream shaderFile(shaderFilePath);

	//TODO: Check whether shader file has actually opened
	std::stringstream ss;

	ss << shaderFile.rdbuf();

	shaderFile.close();

	return ss.str();
}

void ComputeShader::Bind() const
{
	GLCall(glUseProgram(m_RendererID));
}

void ComputeShader::Unbind() const
{
	GLCall(glUseProgram(0));
}

void ComputeShader::Run(unsigned int groupX, unsigned int groupY, unsigned int groupZ) const
{
	GLCall(glDispatchCompute(groupX, groupY, groupZ));
}

void ComputeShader::SetInputUniformImage(std::string name, unsigned int textureId, unsigned int imageUnit, unsigned int channels) const
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
	GLCall(glBindImageTexture(imageUnit, textureId, 0, GL_FALSE, 0, GL_READ_ONLY, channelFlag));
	GLCall(glUniform1i(loc, imageUnit));
}

void ComputeShader::SetOutputUniformImage(std::string name, unsigned int textureId, unsigned int imageUnit, unsigned int channels) const
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
	GLCall(glBindImageTexture(imageUnit, textureId, 0, GL_FALSE, 0, GL_WRITE_ONLY, channelFlag));
	GLCall(glUniform1i(loc, imageUnit));
}

void ComputeShader::SetUniformBlock(std::string name, unsigned int bindPoint)
{
	GLCall(unsigned int loc = glGetUniformBlockIndex(m_RendererID, name.c_str()));
	GLCall(glUniformBlockBinding(m_RendererID, loc, bindPoint));
}

void ComputeShader::SetUniform1i(const std::string& name, int i)
{
	GLCall(glUniform1i(GetUniformLocation(name), i));
}

int ComputeShader::GetUniformLocation(const std::string& name)
{
	GLCall(int location = glGetUniformLocation(m_RendererID, name.c_str()));
	return location;
}
