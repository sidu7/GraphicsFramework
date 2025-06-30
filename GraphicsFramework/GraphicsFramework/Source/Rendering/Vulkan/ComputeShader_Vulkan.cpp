#include "ComputeShader_Vulkan.h"

#include "Core/Core.h"
#include "Rendering/Renderer.h"
#include "Utils/FileHelper.h"
#include "Rendering/Vulkan/Texture_Vulkan.h"

ComputeShader_Vulkan::ComputeShader_Vulkan() 
	: m_RendererID(0)
{
}

ComputeShader_Vulkan::~ComputeShader_Vulkan()
{
}

unsigned int ComputeShader_Vulkan::CompileShader(unsigned int type, std::string& Source)
{
	unsigned int id = 0;
	return id;
}

void ComputeShader_Vulkan::SetShader(std::string path)
{
}

void ComputeShader_Vulkan::Run(unsigned int groupX, unsigned int groupY, unsigned int groupZ) const
{
}

void ComputeShader_Vulkan::SetInputUniformImage(std::string name, const Texture* texture, unsigned int imageUnit, unsigned int channels) const
{
}

void ComputeShader_Vulkan::SetOutputUniformImage(std::string name, const Texture* texture, unsigned int imageUnit, unsigned int channels) const
{
}

void ComputeShader_Vulkan::SetUniformBlock(std::string name, unsigned int bindPoint)
{
}

void ComputeShader_Vulkan::SetUniform1i(const std::string& name, int i)
{
}

void ComputeShader_Vulkan::SetUniform1f(const std::string& name, float v)
{
}

void ComputeShader_Vulkan::SetUniform2f(const std::string& name, float v1, float v2)
{
}

void ComputeShader_Vulkan::SetUniform2f(const std::string& name, glm::vec2 v)
{
}

void ComputeShader_Vulkan::SetUniform3f(const std::string& name, float v1, float v2, float v3)
{
}

void ComputeShader_Vulkan::SetUniform3f(const std::string& name, glm::vec3 v)
{
}

int ComputeShader_Vulkan::GetUniformLocation(const std::string& name)
{
	int location = 0;
	return location;
}
