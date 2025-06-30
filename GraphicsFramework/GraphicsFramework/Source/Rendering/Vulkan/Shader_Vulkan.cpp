/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#include "Shader_Vulkan.h"
#include "Core/Core.h"
#include "Utils/FileHelper.h"

#include "Rendering/ShaderManager.h"
#include "Rendering/Vulkan/VertexBuffer_Vulkan.h"
#include "Rendering/Vulkan/FrameBuffer_Vulkan.h"
#include "Rendering/Vulkan/UniformBuffer_Vulkan.h"
#include "Rendering/Vulkan/Internal/RenderPass_Vulkan.h"

Shader_Vulkan::Shader_Vulkan() :
	mPipeline(VK_NULL_HANDLE),
	mPipelineLayout(VK_NULL_HANDLE),
	mViewPort{},
	mScissorRect{},
	mBoundFramebuffer(nullptr)
{
}

Shader_Vulkan::~Shader_Vulkan()
{
	vkDestroyPipelineLayout(Renderer_Vulkan::Get()->GetDevice(), mPipelineLayout, nullptr);
	vkDestroyPipeline(Renderer_Vulkan::Get()->GetDevice(), mPipeline, nullptr);
}

void Shader_Vulkan::Init(std::string vertexShaderId, std::string fragmentShaderId)
{
	ShaderSource shaders;
	shaders.vertexSource = ShaderManager::Instance()->GetShaderContents(vertexShaderId);
	shaders.fragmentSource = ShaderManager::Instance()->GetShaderContents(fragmentShaderId);

	std::vector<VkPipelineShaderStageCreateInfo> ShaderStageCreateInfos;
	VkShaderModule VertexShaderModule = VK_NULL_HANDLE;
	VkShaderModule FragmentShaderModule = VK_NULL_HANDLE;

	// Programmable stages
	VkShaderModuleCreateInfo VertexShaderModuleCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	VertexShaderModuleCreateInfo.codeSize = shaders.vertexSource.size();
	VertexShaderModuleCreateInfo.pCode = (const uint32_t*)shaders.vertexSource.data();

	VKCall(vkCreateShaderModule(Renderer_Vulkan::Get()->GetDevice(), &VertexShaderModuleCreateInfo, nullptr, &VertexShaderModule), "Vertex Shader Module creation Failed.");

	VkShaderModuleCreateInfo FragmentShaderModuleCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	FragmentShaderModuleCreateInfo.codeSize = shaders.fragmentSource.size();
	FragmentShaderModuleCreateInfo.pCode = (const uint32_t*)shaders.fragmentSource.data();

	VKCall(vkCreateShaderModule(Renderer_Vulkan::Get()->GetDevice(), &FragmentShaderModuleCreateInfo, nullptr, &FragmentShaderModule), "Fragment Shader Module creation Failed.");

	VkPipelineShaderStageCreateInfo VertexShaderStageCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	VertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	VertexShaderStageCreateInfo.module = VertexShaderModule;
	VertexShaderStageCreateInfo.pName = "main";

	ShaderStageCreateInfos.push_back(VertexShaderStageCreateInfo);

	VkPipelineShaderStageCreateInfo FragmentShaderStageCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	FragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	FragmentShaderStageCreateInfo.module = FragmentShaderModule;
	FragmentShaderStageCreateInfo.pName = "main";

	ShaderStageCreateInfos.push_back(FragmentShaderStageCreateInfo);
	
	VkCullModeFlags CullMode = VK_CULL_MODE_BACK_BIT;
	VkFrontFace FrontFace = VK_FRONT_FACE_CLOCKWISE;
	VkBool32 DepthTestEnable = VK_FALSE;

	// Fixed pipeline stages
	VkPipelineVertexInputStateCreateInfo VertexInputStageCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	VertexInputStageCreateInfo.vertexBindingDescriptionCount = VertexBindingDescs.size();
	VertexInputStageCreateInfo.pVertexBindingDescriptions = VertexBindingDescs.data();
	VertexInputStageCreateInfo.vertexAttributeDescriptionCount = VertexAttribDescs.size();
	VertexInputStageCreateInfo.pVertexAttributeDescriptions = VertexAttribDescs.data();

	VkPrimitiveTopology Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	InputAssemblyStateCreateInfo.topology = Topology;
	InputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo ViewportStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	ViewportStateCreateInfo.viewportCount = 1;
	ViewportStateCreateInfo.pViewports = &mViewPort;
	ViewportStateCreateInfo.scissorCount = 1;
	ViewportStateCreateInfo.pScissors = &mScissorRect;

	VkPipelineTessellationStateCreateInfo TessellationStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };

	VkPipelineRasterizationStateCreateInfo RasterizationStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	RasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	RasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	RasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	RasterizationStateCreateInfo.cullMode = CullMode;
	RasterizationStateCreateInfo.frontFace = FrontFace;
	RasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	RasterizationStateCreateInfo.depthBiasConstantFactor = 0.f;
	RasterizationStateCreateInfo.depthBiasClamp = 0.f;
	RasterizationStateCreateInfo.depthBiasSlopeFactor = 0.f;
	RasterizationStateCreateInfo.lineWidth = 1.f;

	VkPipelineMultisampleStateCreateInfo MultisampleStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	MultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	MultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	MultisampleStateCreateInfo.minSampleShading = 1.f;
	MultisampleStateCreateInfo.pSampleMask = nullptr;
	MultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	MultisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo DepthStencilStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	DepthStencilStateCreateInfo.depthTestEnable = VK_FALSE;
	DepthStencilStateCreateInfo.depthWriteEnable = VK_FALSE;
	DepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	DepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
	DepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
	DepthStencilStateCreateInfo.front = VkStencilOpState{};
	DepthStencilStateCreateInfo.back = VkStencilOpState{};
	DepthStencilStateCreateInfo.minDepthBounds = 0.f;
	DepthStencilStateCreateInfo.maxDepthBounds = 1.f;

	VkPipelineColorBlendAttachmentState ColorBlendAttachmentState = {};
	ColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	ColorBlendAttachmentState.blendEnable = VK_FALSE;
	ColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	ColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	ColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	ColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	ColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	ColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo ColorBlendStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	ColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	ColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	ColorBlendStateCreateInfo.attachmentCount = 1;
	ColorBlendStateCreateInfo.pAttachments = &ColorBlendAttachmentState;
	ColorBlendStateCreateInfo.blendConstants[0] = 0.f;
	ColorBlendStateCreateInfo.blendConstants[1] = 0.f;
	ColorBlendStateCreateInfo.blendConstants[2] = 0.f;
	ColorBlendStateCreateInfo.blendConstants[3] = 0.f;

	const std::vector<VkDynamicState> DynamicState = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_CULL_MODE,
		VK_DYNAMIC_STATE_FRONT_FACE,
		VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY,
		VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
	};

	VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	DynamicStateCreateInfo.dynamicStateCount = DynamicState.size();
	DynamicStateCreateInfo.pDynamicStates = DynamicState.data();

	// Create Pipeline Layout
	VkPipelineLayoutCreateInfo PipelineCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	PipelineCreateInfo.flags = VK_PIPELINE_BIND_POINT_GRAPHICS;
	PipelineCreateInfo.setLayoutCount = DescriptorSetLayouts.size();
	PipelineCreateInfo.pSetLayouts = DescriptorSetLayouts.data();
	PipelineCreateInfo.pushConstantRangeCount = 0;
	PipelineCreateInfo.pPushConstantRanges = nullptr;

	VKCall(vkCreatePipelineLayout(Renderer_Vulkan::Get()->GetDevice(), &PipelineCreateInfo, nullptr, &mPipelineLayout), "Pipeline Layout creation Failed.");

	// Create Graphics Pipeline
	VkGraphicsPipelineCreateInfo GraphicsPipeLineCreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	GraphicsPipeLineCreateInfo.stageCount = ShaderStageCreateInfos.size();
	GraphicsPipeLineCreateInfo.pStages = ShaderStageCreateInfos.data();
	GraphicsPipeLineCreateInfo.pVertexInputState = &VertexInputStageCreateInfo;
	GraphicsPipeLineCreateInfo.pInputAssemblyState = &InputAssemblyStateCreateInfo;
	GraphicsPipeLineCreateInfo.pTessellationState = &TessellationStateCreateInfo;
	GraphicsPipeLineCreateInfo.pViewportState = &ViewportStateCreateInfo;
	GraphicsPipeLineCreateInfo.pRasterizationState = &RasterizationStateCreateInfo;
	GraphicsPipeLineCreateInfo.pMultisampleState = &MultisampleStateCreateInfo;
	GraphicsPipeLineCreateInfo.pDepthStencilState = &DepthStencilStateCreateInfo;
	GraphicsPipeLineCreateInfo.pColorBlendState = &ColorBlendStateCreateInfo;
	GraphicsPipeLineCreateInfo.pDynamicState = &DynamicStateCreateInfo;
	GraphicsPipeLineCreateInfo.layout = mPipelineLayout;

	if (mBoundFramebuffer)
	{
		GraphicsPipeLineCreateInfo.renderPass = mBoundFramebuffer->mRenderPass->mRenderPass;
	}
	else // Use BackBuffer's Renderpass
	{
		const FrameBuffer_Vulkan* VkframeBuffer = static_cast<const FrameBuffer_Vulkan*>(Renderer_Vulkan::Get()->GetBackBuffer());
		GraphicsPipeLineCreateInfo.renderPass = VkframeBuffer->mRenderPass->mRenderPass;
	}

	GraphicsPipeLineCreateInfo.subpass = 0;
	GraphicsPipeLineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	GraphicsPipeLineCreateInfo.basePipelineIndex = -1;

	VKCall(vkCreateGraphicsPipelines(Renderer_Vulkan::Get()->GetDevice(), VK_NULL_HANDLE, 1, &GraphicsPipeLineCreateInfo, nullptr, &mPipeline), "Shader Pipeline creation Failed.");

	// Destroy shader modules as graphics pipeline is created
	vkDestroyShaderModule(Renderer_Vulkan::Get()->GetDevice(), VertexShaderModule, nullptr);
	vkDestroyShaderModule(Renderer_Vulkan::Get()->GetDevice(), FragmentShaderModule, nullptr);
}

void Shader_Vulkan::Uses(const FrameBuffer* frameBuffer)
{
	mViewPort.x = 0;
	mViewPort.y = 0;
	mViewPort.width = frameBuffer->GetWidth();
	mViewPort.height = frameBuffer->GetHeight();
	mViewPort.minDepth = 0.0f;
	mViewPort.maxDepth = 1.0f;

	mScissorRect.offset = { 0,0 };
	mScissorRect.extent.width = frameBuffer->GetWidth();
	mScissorRect.extent.height = frameBuffer->GetHeight();

	mBoundFramebuffer = static_cast<const FrameBuffer_Vulkan*>(frameBuffer);
}

void Shader_Vulkan::Uses(const Texture* texture, unsigned int slot)
{
}

void Shader_Vulkan::Uses(const UniformBuffer* uniformBuffer, unsigned int binding)
{
	const UniformBuffer_Vulkan* VkUbo = static_cast<const UniformBuffer_Vulkan*>(uniformBuffer);
	if (VkUbo)
	{
		DescriptorSetLayouts.push_back(VkUbo->UboDescSet->DescLayout);
		const_cast<UniformBuffer_Vulkan*>(VkUbo)->PipelineLayout = &mPipelineLayout;
	}
}

void Shader_Vulkan::Uses(const VertexBuffer* vertexBuffer)
{
	struct Vertex
	{
		glm::vec2 Position;
		glm::vec3 Color;
	};

	VertexBindingDescs.resize(1);
	VertexBindingDescs[0].binding = 0;
	VertexBindingDescs[0].stride = vertexBuffer->mStride;
	VertexBindingDescs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	uint16_t Location = 0;
	for (const VertexData& vertexData : vertexBuffer->mVertexLayouts)
	{
		VkVertexInputAttributeDescription VertexAttribDesc = {};
		VertexAttribDesc.binding = 0;
		VertexAttribDesc.location = Location++;
		VertexAttribDesc.format = VertexBuffer_Vulkan::GetVertexFormat(vertexData.mFormat);
		VertexAttribDesc.offset = vertexData.mOffset;

		VertexAttribDescs.push_back(VertexAttribDesc);
	}
}

void Shader_Vulkan::Uses(const IndexBuffer* indexBuffer)
{
}