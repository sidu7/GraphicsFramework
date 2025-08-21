/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

#include "Rendering/Shader.h"

#include "Rendering/Vulkan/Renderer_Vulkan.h"

class DescriptorSet_Vulkan;

class Shader_Vulkan : public Shader
{
public:
	Shader_Vulkan();
	virtual ~Shader_Vulkan();

	virtual void Init(std::string shaderId) override;

	VkPipeline mPipeline;
	VkPipelineLayout mPipelineLayout;
	DescriptorSet_Vulkan* mDescriptorSet;

protected:
	void GenerateShaderBindings(const std::vector<char>& shaderSource);

	std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
	std::vector<VkVertexInputBindingDescription> VertexBindingDescs;
	std::vector<VkVertexInputAttributeDescription> VertexAttribDescs;
	const FrameBuffer_Vulkan* mBoundFramebuffer;
	std::string mShaderId;
};