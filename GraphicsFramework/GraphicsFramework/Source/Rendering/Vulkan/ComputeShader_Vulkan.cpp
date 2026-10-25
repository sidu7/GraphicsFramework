/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#include "ComputeShader_Vulkan.h"

#include "Core/Core.h"
#include "Utils/FileHelper.h"
#include "Rendering/Vulkan/Texture_Vulkan.h"
#include "Rendering/Vulkan/Internal/VulkanHelper.h"

#include <spirv_reflect\spirv_reflect.h>

ComputeShader_Vulkan::ComputeShader_Vulkan()
	: mPipeline(VK_NULL_HANDLE), mPipelineLayout(VK_NULL_HANDLE)
{
}

ComputeShader_Vulkan::~ComputeShader_Vulkan()
{
	if (mPipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(Renderer_Vulkan::Get()->GetDevice(), mPipeline, nullptr);
	}
	if (mPipelineLayout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(Renderer_Vulkan::Get()->GetDevice(), mPipelineLayout, nullptr);
	}
}

void ComputeShader_Vulkan::Init(int32_t shaderId)
{
	mShaderId = std::to_string(shaderId);

	const auto& path = ShaderRegistry::Instance()->GetSPIRVComputePath(shaderId);
	std::vector<char> shaderSource = FileHelper::ReadBinaryFile(path);

	if (shaderSource.empty())
	{
		std::cout << "Failed to load SPIR-V compute shader for: " << mShaderId << std::endl;
		return;
	}

	// Create shader module from SPIR-V
	VkShaderModule shaderModule = VK_NULL_HANDLE;
	LoadShaderModule(shaderSource, shaderModule);

	// Create pipeline layout from SPIR-V reflection
	VkDescriptorSetLayout descSetLayout;
	{
		SpvReflectShaderModule reflectModule;
		SpvReflectResult result = spvReflectCreateShaderModule(shaderSource.size(), shaderSource.data(), &reflectModule);
		if (result == SPV_REFLECT_RESULT_SUCCESS)
		{
			uint32_t descCount = 0;
			spvReflectEnumerateDescriptorSets(&reflectModule, &descCount, nullptr);
			std::vector<SpvReflectDescriptorSet*> descSets(descCount);
			if (spvReflectEnumerateDescriptorSets(&reflectModule, &descCount, descSets.data()) == SPV_REFLECT_RESULT_SUCCESS)
			{
				for (uint32_t i = 0; i < descCount; ++i)
				{
					const auto& descSet = *descSets[i];
					VkDescriptorSetLayoutCreateInfo layoutInfo = {};
					layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					layoutInfo.bindingCount = descSet.binding_count;
					layoutInfo.pBindings = descSet.bindings;

					VkDevice device = Renderer_Vulkan::Get()->GetDevice();
					VKCall(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descSetLayout), "Compute shader descriptor set layout creation failed.");

					VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
					pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
					pipelineLayoutInfo.setLayoutCount = 1;
					pipelineLayoutInfo.pSetLayouts = &descSetLayout;
					pipelineLayoutInfo.pushConstantRangeCount = 0;

					VKCall(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &mPipelineLayout), "Compute shader pipeline layout creation failed.");
					break;
				}
			}
			spvReflectDestroyShaderModule(&reflectModule);
		}
	}

	// Create compute pipeline
	VkPipelineShaderStageCreateInfo stageCreateInfo = {};
	stageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stageCreateInfo.module = shaderModule;
	stageCreateInfo.pName = "main";

	VkComputePipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stage = stageCreateInfo;
	pipelineCreateInfo.layout = mPipelineLayout;

	VkDevice device = Renderer_Vulkan::Get()->GetDevice();
	VKCall(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &mPipeline), "Compute shader pipeline creation failed.");

	vkDestroyShaderModule(device, shaderModule, nullptr);
}

void ComputeShader_Vulkan::Init(const std::string& filePath)
{
	mShaderId = filePath;

	std::vector<char> shaderSource = FileHelper::ReadBinaryFile(filePath + ".spv");

	if (shaderSource.empty())
	{
		std::cout << "Failed to load SPIR-V compute shader from path: " << filePath << std::endl;
		return;
	}

	// Create shader module from SPIR-V
	VkShaderModule shaderModule = VK_NULL_HANDLE;
	LoadShaderModule(shaderSource, shaderModule);

	// Create pipeline layout
	VkDescriptorSetLayout descSetLayout;
	{
		SpvReflectShaderModule reflectModule;
		SpvReflectResult result = spvReflectCreateShaderModule(shaderSource.size(), shaderSource.data(), &reflectModule);
		if (result == SPV_REFLECT_RESULT_SUCCESS)
		{
			uint32_t descCount = 0;
			spvReflectEnumerateDescriptorSets(&reflectModule, &descCount, nullptr);
			std::vector<SpvReflectDescriptorSet*> descSets(descCount);
			if (spvReflectEnumerateDescriptorSets(&reflectModule, &descCount, descSets.data()) == SPV_REFLECT_RESULT_SUCCESS)
			{
				for (uint32_t i = 0; i < descCount; ++i)
				{
					const auto& descSet = *descSets[i];
					VkDescriptorSetLayoutCreateInfo layoutInfo = {};
					layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					layoutInfo.bindingCount = descSet.binding_count;
					layoutInfo.pBindings = descSet.bindings;

					VkDevice device = Renderer_Vulkan::Get()->GetDevice();
					VKCall(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descSetLayout), "Compute shader descriptor set layout creation failed.");

					VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
					pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
					pipelineLayoutInfo.setLayoutCount = 1;
					pipelineLayoutInfo.pSetLayouts = &descSetLayout;
					pipelineLayoutInfo.pushConstantRangeCount = 0;

					VKCall(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &mPipelineLayout), "Compute shader pipeline layout creation failed.");
					break;
				}
			}
			spvReflectDestroyShaderModule(&reflectModule);
		}
	}

	// Create compute pipeline
	VkPipelineShaderStageCreateInfo stageCreateInfo = {};
	stageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stageCreateInfo.module = shaderModule;
	stageCreateInfo.pName = "main";

	VkComputePipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stage = stageCreateInfo;
	pipelineCreateInfo.layout = mPipelineLayout;

	VkDevice device = Renderer_Vulkan::Get()->GetDevice();
	VKCall(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &mPipeline), "Compute shader pipeline creation failed.");

	vkDestroyShaderModule(device, shaderModule, nullptr);
}

void ComputeShader_Vulkan::LoadShaderModule(const std::vector<char>& shaderSource, VkShaderModule& outModule)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shaderSource.size();
	createInfo.pCode = (const uint32_t*)shaderSource.data();

	VkDevice device = Renderer_Vulkan::Get()->GetDevice();
	VKCall(vkCreateShaderModule(device, &createInfo, nullptr, &outModule), "Compute shader module creation failed.");
}

void ComputeShader_Vulkan::Run(unsigned int groupX, unsigned int groupY, unsigned int groupZ) const
{
	VkCommandBuffer commandBuffer = Renderer_Vulkan::Get()->BeginSingleTimeCommands();

	VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
	vkCmdBindPipeline(commandBuffer, bindPoint, mPipeline);
	vkCmdDispatch(commandBuffer, groupX, groupY, groupZ);

	Renderer_Vulkan::Get()->EndSingleTimeCommands(commandBuffer);
}

void ComputeShader_Vulkan::SetInputUniformImage(std::string name, const Texture* texture, unsigned int imageUnit, unsigned int channels) const
{
	// Placeholder - requires descriptor set binding management in Vulkan
	(void)name;
	(void)texture;
	(void)imageUnit;
	(void)channels;
}

void ComputeShader_Vulkan::SetOutputUniformImage(std::string name, const Texture* texture, unsigned int imageUnit, unsigned int channels) const
{
	// Placeholder - requires descriptor set binding management in Vulkan
	(void)name;
	(void)texture;
	(void)imageUnit;
	(void)channels;
}

void ComputeShader_Vulkan::SetUniformBlock(std::string name, unsigned int bindPoint)
{
	// Placeholder - requires descriptor set binding management in Vulkan
	(void)name;
	(void)bindPoint;
}

void ComputeShader_Vulkan::SetUniform1i(const std::string& name, int i)
{
	(void)name;
	(void)i;
}

void ComputeShader_Vulkan::SetUniform1f(const std::string& name, float v)
{
	(void)name;
	(void)v;
}

void ComputeShader_Vulkan::SetUniform2f(const std::string& name, float v1, float v2)
{
	(void)name;
	(void)v1;
	(void)v2;
}

void ComputeShader_Vulkan::SetUniform2f(const std::string& name, glm::vec2 v)
{
	(void)name;
	(void)v;
}

void ComputeShader_Vulkan::SetUniform3f(const std::string& name, float v1, float v2, float v3)
{
	(void)name;
	(void)v1;
	(void)v2;
	(void)v3;
}

void ComputeShader_Vulkan::SetUniform3f(const std::string& name, glm::vec3 v)
{
	(void)name;
	(void)v;
}

int ComputeShader_Vulkan::GetUniformLocation(const std::string& name)
{
	(void)name;
	return 0;
}
