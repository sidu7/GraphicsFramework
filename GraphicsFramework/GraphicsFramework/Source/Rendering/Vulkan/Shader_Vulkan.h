/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

#include "Rendering/Shader.h"
#include "Rendering/ShaderRegistry.h"

#include "Rendering/Vulkan/Renderer_Vulkan.h"

class DescriptorSet_Vulkan;

class Shader_Vulkan : public Shader
{
public:
	Shader_Vulkan();
	virtual ~Shader_Vulkan();

	virtual void Init(int32_t shaderId) override;
	virtual void Init(const std::string& vertexPath, const std::string& fragmentPath) override;

	VkPipeline mPipeline;
	VkPipelineLayout mPipelineLayout;
	DescriptorSet_Vulkan* mDescriptorSet;

protected:
	void GenerateShaderBindings(const std::vector<char>& shaderSource, ShaderRegistry::ShaderStage stage);
	void LoadShaderModule(const std::vector<char>& shaderSource, VkShaderModule& outModule);

	std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
	std::vector<VkVertexInputBindingDescription> VertexBindingDescs;
	std::vector<VkVertexInputAttributeDescription> VertexAttribDescs;
	const FrameBuffer_Vulkan* mBoundFramebuffer;
	std::string mShaderId;
};
