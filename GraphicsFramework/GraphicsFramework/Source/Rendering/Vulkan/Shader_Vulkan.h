/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

#include "Rendering/Shader.h"

#include "Rendering/Vulkan/Renderer_Vulkan.h"

class Shader_Vulkan : public Shader
{
public:
	Shader_Vulkan();
	virtual ~Shader_Vulkan();

	virtual void Uses(const FrameBuffer* framebuffer) override;
	virtual void Uses(const Texture* texture, unsigned int slot = 0) override;
	virtual void Uses(const UniformBuffer* uniformBuffer, unsigned int binding = 0) override;
	virtual void Uses(const VertexBuffer* vertexBuffer) override;
	virtual void Uses(const IndexBuffer* indexBuffer) override;

	virtual void Init(std::string vertexShaderId, std::string fragmentShaderId) override;

	VkPipeline mPipeline;
	VkPipelineLayout mPipelineLayout;

protected:
	std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
	std::vector<VkVertexInputBindingDescription> VertexBindingDescs;
	std::vector<VkVertexInputAttributeDescription> VertexAttribDescs;
	VkViewport mViewPort;
	VkRect2D mScissorRect;
	const FrameBuffer_Vulkan* mBoundFramebuffer;
};