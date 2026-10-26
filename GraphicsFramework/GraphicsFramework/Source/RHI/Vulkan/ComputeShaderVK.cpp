/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#include "ComputeShaderVK.h"

#include "Core/Core.h"
#include "Utils/FileHelper.h"
#include "RHI/Vulkan/TextureVK.h"
#include "RHI/Vulkan/Internal/CommandBufferVK.h"
#include "RHI/Vulkan/Internal/CommandQueueVK.h"
#include "RHI/Vulkan/Internal/HelperVK.h"

#include <spirv_reflect\spirv_reflect.h>

namespace rhi::vk
{
ComputeShaderVK::ComputeShaderVK()
	: mPipeline(VK_NULL_HANDLE), mPipelineLayout(VK_NULL_HANDLE)
{
}

ComputeShaderVK::~ComputeShaderVK()
{
	if (mPipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(Device::Get()->GetDevice(), mPipeline, nullptr);
	}
	if (mPipelineLayout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(Device::Get()->GetDevice(), mPipelineLayout, nullptr);
	}
}

void ComputeShaderVK::Init(int32_t shaderId)
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

					// spv-reflect exposes its own binding structs; they are not layout
					// compatible with VkDescriptorSetLayoutBinding, so convert them.
					std::vector<VkDescriptorSetLayoutBinding> bindings(descSet.binding_count);
					for (uint32_t b = 0; b < descSet.binding_count; ++b)
					{
						const SpvReflectDescriptorBinding& reflectedBinding = *descSet.bindings[b];
						VkDescriptorSetLayoutBinding& binding = bindings[b];
						binding = {};
						binding.binding = reflectedBinding.binding;
						binding.descriptorType = static_cast<VkDescriptorType>(reflectedBinding.descriptor_type);
						binding.descriptorCount = 1;
						for (uint32_t dim = 0; dim < reflectedBinding.array.dims_count; ++dim)
						{
							binding.descriptorCount *= reflectedBinding.array.dims[dim];
						}
						binding.stageFlags = static_cast<VkShaderStageFlagBits>(reflectModule.shader_stage);
						binding.pImmutableSamplers = nullptr;
					}

					VkDescriptorSetLayoutCreateInfo layoutInfo = {};
					layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
					layoutInfo.pBindings = bindings.data();

					VkDevice device = Device::Get()->GetDevice();
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

	VkDevice device = Device::Get()->GetDevice();
	VKCall(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &mPipeline), "Compute shader pipeline creation failed.");

	vkDestroyShaderModule(device, shaderModule, nullptr);
}

void ComputeShaderVK::Init(const std::string& filePath)
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

					// spv-reflect exposes its own binding structs; they are not layout
					// compatible with VkDescriptorSetLayoutBinding, so convert them.
					std::vector<VkDescriptorSetLayoutBinding> bindings(descSet.binding_count);
					for (uint32_t b = 0; b < descSet.binding_count; ++b)
					{
						const SpvReflectDescriptorBinding& reflectedBinding = *descSet.bindings[b];
						VkDescriptorSetLayoutBinding& binding = bindings[b];
						binding = {};
						binding.binding = reflectedBinding.binding;
						binding.descriptorType = static_cast<VkDescriptorType>(reflectedBinding.descriptor_type);
						binding.descriptorCount = 1;
						for (uint32_t dim = 0; dim < reflectedBinding.array.dims_count; ++dim)
						{
							binding.descriptorCount *= reflectedBinding.array.dims[dim];
						}
						binding.stageFlags = static_cast<VkShaderStageFlagBits>(reflectModule.shader_stage);
						binding.pImmutableSamplers = nullptr;
					}

					VkDescriptorSetLayoutCreateInfo layoutInfo = {};
					layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
					layoutInfo.pBindings = bindings.data();

					VkDevice device = Device::Get()->GetDevice();
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

	VkDevice device = Device::Get()->GetDevice();
	VKCall(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &mPipeline), "Compute shader pipeline creation failed.");

	vkDestroyShaderModule(device, shaderModule, nullptr);
}

void ComputeShaderVK::LoadShaderModule(const std::vector<char>& shaderSource, VkShaderModule& outModule)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shaderSource.size();
	createInfo.pCode = (const uint32_t*)shaderSource.data();

	VkDevice device = Device::Get()->GetDevice();
	VKCall(vkCreateShaderModule(device, &createInfo, nullptr, &outModule), "Compute shader module creation failed.");
}

void ComputeShaderVK::Run(unsigned int groupX, unsigned int groupY, unsigned int groupZ) const
{
	// Single-time submission: record the dispatch, submit it and wait for completion.
	CommandBufferVK* commandBuffer = Device::Get()->AllocateTransientCommandBuffer(CommandQueueType::Graphics);
	commandBuffer->Begin();

	vkCmdBindPipeline(commandBuffer->mCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mPipeline);
	vkCmdDispatch(commandBuffer->mCommandBuffer, groupX, groupY, groupZ);

	commandBuffer->End();
	Device::Get()->GetQueue(CommandQueueType::Graphics)->SubmitCommandBuffer(commandBuffer, true);

	delete commandBuffer;
}

void ComputeShaderVK::SetInputUniformImage(std::string name, const Texture* texture, unsigned int imageUnit, unsigned int channels) const
{
	// Placeholder - requires descriptor set binding management in Vulkan
	(void)name;
	(void)texture;
	(void)imageUnit;
	(void)channels;
}

void ComputeShaderVK::SetOutputUniformImage(std::string name, const Texture* texture, unsigned int imageUnit, unsigned int channels) const
{
	// Placeholder - requires descriptor set binding management in Vulkan
	(void)name;
	(void)texture;
	(void)imageUnit;
	(void)channels;
}

void ComputeShaderVK::SetUniformBlock(std::string name, unsigned int bindPoint)
{
	// Placeholder - requires descriptor set binding management in Vulkan
	(void)name;
	(void)bindPoint;
}

void ComputeShaderVK::SetUniform1i(const std::string& name, int i)
{
	(void)name;
	(void)i;
}

void ComputeShaderVK::SetUniform1f(const std::string& name, float v)
{
	(void)name;
	(void)v;
}

void ComputeShaderVK::SetUniform2f(const std::string& name, float v1, float v2)
{
	(void)name;
	(void)v1;
	(void)v2;
}

void ComputeShaderVK::SetUniform2f(const std::string& name, glm::vec2 v)
{
	(void)name;
	(void)v;
}

void ComputeShaderVK::SetUniform3f(const std::string& name, float v1, float v2, float v3)
{
	(void)name;
	(void)v1;
	(void)v2;
	(void)v3;
}

void ComputeShaderVK::SetUniform3f(const std::string& name, glm::vec3 v)
{
	(void)name;
	(void)v;
}

int ComputeShaderVK::GetUniformLocation(const std::string& name)
{
	(void)name;
	return 0;
}
}
