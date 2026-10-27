/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#include "ShaderVK.h"
#include "Core/Core.h"
#include "Utils/FileHelper.h"

#include "RHI/Vulkan/VertexBufferVK.h"
#include "RHI/Vulkan/FrameBufferVK.h"
#include "RHI/Vulkan/Internal/RenderPassVK.h"
#include "RHI/Vulkan/Internal/DescriptorSetVK.h"
#include "RHI/Vulkan/Internal/HelperVK.h"

#include <spirv_reflect\spirv_reflect.h>

namespace rhi::vk
{
ShaderVK::ShaderVK() :
	mPipeline(VK_NULL_HANDLE),
	mLinePipeline(VK_NULL_HANDLE),
	mPipelineLayout(VK_NULL_HANDLE),
	mDescriptorSet(nullptr),
	mBoundFramebuffer(nullptr)
{
}

ShaderVK::~ShaderVK()
{
	if (mPipelineLayout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(Device::Get()->GetDevice(), mPipelineLayout, nullptr);
	}
	if (mPipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(Device::Get()->GetDevice(), mPipeline, nullptr);
	}
	if (mLinePipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(Device::Get()->GetDevice(), mLinePipeline, nullptr);
	}
	delete mDescriptorSet;
}

void ShaderVK::Init(int32_t shaderId)
{
	mShaderId = std::to_string(shaderId);

	const auto& vertPath = ShaderRegistry::Instance()->GetSPIRVPath(shaderId, ShaderRegistry::ShaderStage::Vertex);
	const auto& fragPath = ShaderRegistry::Instance()->GetSPIRVPath(shaderId, ShaderRegistry::ShaderStage::Fragment);

	std::vector<char> vertSource = FileHelper::ReadBinaryFile(vertPath);
	std::vector<char> fragSource = FileHelper::ReadBinaryFile(fragPath);

	if (vertSource.empty() || fragSource.empty())
	{
		std::cout << "Failed to load SPIR-V shader source for: " << mShaderId << std::endl;
		return;
	}

	std::vector<VkPipelineShaderStageCreateInfo> ShaderStageCreateInfos;

	mDescriptorSet = new DescriptorSetVK();

	// Get Shader variables info using SPIRV-Reflect
	GenerateShaderBindings(vertSource, ShaderRegistry::ShaderStage::Vertex);
	GenerateShaderBindings(fragSource, ShaderRegistry::ShaderStage::Fragment);

	// Only create descriptor set if the shader has bindings (uniforms/textures)
	if (mDescriptorSet->GetBindingCount() > 0)
	{
		mDescriptorSet->Init();

		// The set layout only exists after Init(); the pipeline layout must
		// reference the real handle, not the VK_NULL_HANDLE seen while reflecting.
		DescriptorSetLayouts.clear();
		DescriptorSetLayouts.push_back(mDescriptorSet->DescLayout);
	}
	else
	{
		delete mDescriptorSet;
		mDescriptorSet = nullptr;
		DescriptorSetLayouts.clear();
	}

	// Create shader modules from SPIR-V
	VkShaderModule VertexShaderModule = VK_NULL_HANDLE;
	LoadShaderModule(vertSource, VertexShaderModule);

	VkShaderModule FragmentShaderModule = VK_NULL_HANDLE;
	LoadShaderModule(fragSource, FragmentShaderModule);

	// Vertex shader stage
	VkPipelineShaderStageCreateInfo VertexShaderStageCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	VertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	VertexShaderStageCreateInfo.module = VertexShaderModule;
	VertexShaderStageCreateInfo.pName = "main";
	ShaderStageCreateInfos.push_back(VertexShaderStageCreateInfo);

	// Fragment shader stage
	VkPipelineShaderStageCreateInfo FragmentShaderStageCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	FragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	FragmentShaderStageCreateInfo.module = FragmentShaderModule;
	FragmentShaderStageCreateInfo.pName = "main";
	ShaderStageCreateInfos.push_back(FragmentShaderStageCreateInfo);

	VkCullModeFlags CullMode = VK_CULL_MODE_BACK_BIT;
	VkFrontFace FrontFace = VK_FRONT_FACE_CLOCKWISE;

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

	VkViewport Viewport;
	Rect3D RendererViewport = Device::Get()->GetViewportSize();
	Viewport.x = RendererViewport.Offset.x;
	Viewport.y = RendererViewport.Offset.y;
	Viewport.width = RendererViewport.Size.x;
	Viewport.height = RendererViewport.Size.y;
	Viewport.minDepth = RendererViewport.Depth.x;
	Viewport.maxDepth = RendererViewport.Depth.y;

	VkRect2D Scissor;
	Rect2D RendererScissor = Device::Get()->GetScissorSize();
	Scissor.offset.x = RendererViewport.Offset.x;
	Scissor.offset.y = RendererViewport.Offset.y;
	Scissor.extent.width = RendererViewport.Size.x;
	Scissor.extent.height = RendererViewport.Size.y;

	VkPipelineViewportStateCreateInfo ViewportStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	ViewportStateCreateInfo.viewportCount = 1;
	ViewportStateCreateInfo.pViewports = &Viewport;
	ViewportStateCreateInfo.scissorCount = 1;
	ViewportStateCreateInfo.pScissors = &Scissor;

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

	// The back buffer owns a depth attachment; offscreen targets may not.
	const FrameBufferVK* TargetFrameBuffer = mBoundFramebuffer
		? mBoundFramebuffer
		: static_cast<const FrameBufferVK*>(Device::Get()->GetBackBuffer());
	const bool bHasDepthAttachment = TargetFrameBuffer && TargetFrameBuffer->HasDepth();

	VkPipelineDepthStencilStateCreateInfo DepthStencilStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	DepthStencilStateCreateInfo.depthTestEnable = VK_FALSE;
	DepthStencilStateCreateInfo.depthWriteEnable = bHasDepthAttachment ? VK_TRUE : VK_FALSE;
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
	PipelineCreateInfo.flags = 0;
	PipelineCreateInfo.setLayoutCount = DescriptorSetLayouts.size();
	PipelineCreateInfo.pSetLayouts = DescriptorSetLayouts.data();
	PipelineCreateInfo.pushConstantRangeCount = 0;
	PipelineCreateInfo.pPushConstantRanges = nullptr;

	VKCall(vkCreatePipelineLayout(Device::Get()->GetDevice(), &PipelineCreateInfo, nullptr, &mPipelineLayout), "Pipeline Layout creation Failed.");

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
	GraphicsPipeLineCreateInfo.renderPass = TargetFrameBuffer->mRenderPass->mRenderPass;

	GraphicsPipeLineCreateInfo.subpass = 0;
	GraphicsPipeLineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	GraphicsPipeLineCreateInfo.basePipelineIndex = -1;

	VKCall(vkCreateGraphicsPipelines(Device::Get()->GetDevice(), VK_NULL_HANDLE, 1, &GraphicsPipeLineCreateInfo, nullptr, &mPipeline), "Shader Pipeline creation Failed.");

	// Line topology variant for DebugDrawLines()/DebugDraw().
	InputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	VKCall(vkCreateGraphicsPipelines(Device::Get()->GetDevice(), VK_NULL_HANDLE, 1, &GraphicsPipeLineCreateInfo, nullptr, &mLinePipeline), "Shader line Pipeline creation Failed.");

	// Destroy shader modules as graphics pipeline is created
	vkDestroyShaderModule(Device::Get()->GetDevice(), VertexShaderModule, nullptr);
	vkDestroyShaderModule(Device::Get()->GetDevice(), FragmentShaderModule, nullptr);
}

void ShaderVK::Init(const std::string& vertexPath, const std::string& fragmentPath)
{
	mShaderId = vertexPath;

	auto vertSource = FileHelper::ReadBinaryFile(vertexPath + ".spv");
	auto fragSource = FileHelper::ReadBinaryFile(fragmentPath + ".spv");

	if (vertSource.empty() || fragSource.empty())
	{
		std::cout << "Failed to load SPIR-V shader from paths: " << vertexPath << ", " << fragmentPath << std::endl;
		return;
	}

	std::vector<VkPipelineShaderStageCreateInfo> ShaderStageCreateInfos;

	mDescriptorSet = new DescriptorSetVK();

	GenerateShaderBindings(vertSource, ShaderRegistry::ShaderStage::Vertex);
	GenerateShaderBindings(fragSource, ShaderRegistry::ShaderStage::Fragment);

	// Only create descriptor set if the shader has bindings (uniforms/textures)
	if (mDescriptorSet->GetBindingCount() > 0)
	{
		mDescriptorSet->Init();

		DescriptorSetLayouts.clear();
		DescriptorSetLayouts.push_back(mDescriptorSet->DescLayout);
	}
	else
	{
		delete mDescriptorSet;
		mDescriptorSet = nullptr;
		DescriptorSetLayouts.clear();
	}

	VkShaderModule VertexShaderModule = VK_NULL_HANDLE;
	LoadShaderModule(vertSource, VertexShaderModule);

	VkShaderModule FragmentShaderModule = VK_NULL_HANDLE;
	LoadShaderModule(fragSource, FragmentShaderModule);

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

	VkPipelineVertexInputStateCreateInfo VertexInputStageCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	VertexInputStageCreateInfo.vertexBindingDescriptionCount = VertexBindingDescs.size();
	VertexInputStageCreateInfo.pVertexBindingDescriptions = VertexBindingDescs.data();
	VertexInputStageCreateInfo.vertexAttributeDescriptionCount = VertexAttribDescs.size();
	VertexInputStageCreateInfo.pVertexAttributeDescriptions = VertexAttribDescs.data();

	VkPrimitiveTopology Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	InputAssemblyStateCreateInfo.topology = Topology;
	InputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport Viewport;
	Rect3D RendererViewport = Device::Get()->GetViewportSize();
	Viewport.x = RendererViewport.Offset.x;
	Viewport.y = RendererViewport.Offset.y;
	Viewport.width = RendererViewport.Size.x;
	Viewport.height = RendererViewport.Size.y;
	Viewport.minDepth = RendererViewport.Depth.x;
	Viewport.maxDepth = RendererViewport.Depth.y;

	VkRect2D Scissor;
	Rect2D RendererScissor = Device::Get()->GetScissorSize();
	Scissor.offset.x = RendererViewport.Offset.x;
	Scissor.offset.y = RendererViewport.Offset.y;
	Scissor.extent.width = RendererViewport.Size.x;
	Scissor.extent.height = RendererViewport.Size.y;

	VkPipelineViewportStateCreateInfo ViewportStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	ViewportStateCreateInfo.viewportCount = 1;
	ViewportStateCreateInfo.pViewports = &Viewport;
	ViewportStateCreateInfo.scissorCount = 1;
	ViewportStateCreateInfo.pScissors = &Scissor;

	VkPipelineTessellationStateCreateInfo TessellationStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };

	VkPipelineRasterizationStateCreateInfo RasterizationStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	RasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	RasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	RasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	RasterizationStateCreateInfo.cullMode = CullMode;
	RasterizationStateCreateInfo.frontFace = FrontFace;
	RasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	RasterizationStateCreateInfo.lineWidth = 1.f;

	VkPipelineMultisampleStateCreateInfo MultisampleStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	MultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	MultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	MultisampleStateCreateInfo.minSampleShading = 1.f;
	MultisampleStateCreateInfo.pSampleMask = nullptr;
	MultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	MultisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

	// The back buffer owns a depth attachment; offscreen targets may not.
	const FrameBufferVK* TargetFrameBuffer = mBoundFramebuffer
		? mBoundFramebuffer
		: static_cast<const FrameBufferVK*>(Device::Get()->GetBackBuffer());
	const bool bHasDepthAttachment = TargetFrameBuffer && TargetFrameBuffer->HasDepth();

	VkPipelineDepthStencilStateCreateInfo DepthStencilStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	DepthStencilStateCreateInfo.depthTestEnable = VK_FALSE;
	DepthStencilStateCreateInfo.depthWriteEnable = bHasDepthAttachment ? VK_TRUE : VK_FALSE;
	DepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	DepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
	DepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
	DepthStencilStateCreateInfo.front = VkStencilOpState{};
	DepthStencilStateCreateInfo.back = VkStencilOpState{};

	VkPipelineColorBlendAttachmentState ColorBlendAttachmentState = {};
	ColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	ColorBlendAttachmentState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo ColorBlendStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	ColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	ColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	ColorBlendStateCreateInfo.attachmentCount = 1;
	ColorBlendStateCreateInfo.pAttachments = &ColorBlendAttachmentState;

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

	VkPipelineLayoutCreateInfo PipelineCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	PipelineCreateInfo.flags = 0;
	PipelineCreateInfo.setLayoutCount = DescriptorSetLayouts.size();
	PipelineCreateInfo.pSetLayouts = DescriptorSetLayouts.data();
	PipelineCreateInfo.pushConstantRangeCount = 0;
	PipelineCreateInfo.pPushConstantRanges = nullptr;

	VKCall(vkCreatePipelineLayout(Device::Get()->GetDevice(), &PipelineCreateInfo, nullptr, &mPipelineLayout), "Pipeline Layout creation Failed.");

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
	GraphicsPipeLineCreateInfo.renderPass = TargetFrameBuffer->mRenderPass->mRenderPass;

	GraphicsPipeLineCreateInfo.subpass = 0;
	GraphicsPipeLineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	GraphicsPipeLineCreateInfo.basePipelineIndex = -1;

	VKCall(vkCreateGraphicsPipelines(Device::Get()->GetDevice(), VK_NULL_HANDLE, 1, &GraphicsPipeLineCreateInfo, nullptr, &mPipeline), "Shader Pipeline creation Failed.");

	// Line topology variant for DebugDrawLines()/DebugDraw().
	InputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	VKCall(vkCreateGraphicsPipelines(Device::Get()->GetDevice(), VK_NULL_HANDLE, 1, &GraphicsPipeLineCreateInfo, nullptr, &mLinePipeline), "Shader line Pipeline creation Failed.");

	vkDestroyShaderModule(Device::Get()->GetDevice(), VertexShaderModule, nullptr);
	vkDestroyShaderModule(Device::Get()->GetDevice(), FragmentShaderModule, nullptr);
}

void ShaderVK::LoadShaderModule(const std::vector<char>& shaderSource, VkShaderModule& outModule)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shaderSource.size();
	createInfo.pCode = (const uint32_t*)shaderSource.data();

	VKCall(vkCreateShaderModule(Device::Get()->GetDevice(), &createInfo, nullptr, &outModule), "Shader Module creation Failed.");
}

void ShaderVK::GenerateShaderBindings(const std::vector<char>& shaderSource, ShaderRegistry::ShaderStage stage)
{
	if (shaderSource.size() <= 0)
	{
		std::cout << "Shader source is empty for " << mShaderId << std::endl;
		return;
	}

	SpvReflectShaderModule ShaderModule;
	SpvReflectResult SpvResult = spvReflectCreateShaderModule(shaderSource.size(), shaderSource.data(), &ShaderModule);
	if (SpvResult != SPV_REFLECT_RESULT_SUCCESS)
	{
		std::cout << "Failed to create reflection module for " << mShaderId << std::endl;
		return;
	}

	// Inputs (only vertex shader has vertex inputs)
	if (stage == ShaderRegistry::ShaderStage::Vertex)
	{
		uint32_t InputCount = 0;
		spvReflectEnumerateInputVariables(&ShaderModule, &InputCount, nullptr);

		std::vector<SpvReflectInterfaceVariable*> InputVars(InputCount);
		if (spvReflectEnumerateInputVariables(&ShaderModule, &InputCount, InputVars.data()) == SPV_REFLECT_RESULT_SUCCESS)
		{
			for (size_t InputIdx = 0; InputIdx < InputVars.size(); ++InputIdx)
			{
				const SpvReflectInterfaceVariable& ReflectInputVar = *(InputVars[InputIdx]);

				// ignore built-in variables
				if (ReflectInputVar.decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN)
				{
					continue;
				}

				VkVertexInputAttributeDescription VertexAttribDesc = {};
				VertexAttribDesc.binding = 0;
				VertexAttribDesc.location = ReflectInputVar.location;
				VertexAttribDesc.format = static_cast<VkFormat>(ReflectInputVar.format);
				VertexAttribDesc.offset = 0;

				VertexAttribDescs.push_back(VertexAttribDesc);
			}

			if (VertexAttribDescs.size() > 0)
			{
				VertexBindingDescs.resize(1);
				VertexBindingDescs[0].binding = 0;
				VertexBindingDescs[0].stride = 0;
				VertexBindingDescs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

				// Sort attributes by location
				std::sort(std::begin(VertexAttribDescs), std::end(VertexAttribDescs),
					[](const VkVertexInputAttributeDescription& a, const VkVertexInputAttributeDescription& b)
					{
						return a.location < b.location;
					});

				// Compute final offsets of each attribute, and total vertex stride.
				for (auto& attribute : VertexAttribDescs)
				{
					uint32_t format_size = Helper::GetVkFormatSize(attribute.format);
					attribute.offset = VertexBindingDescs[0].stride;
					VertexBindingDescs[0].stride += format_size;
				}
			}
		}
	}

	// Descriptor Sets
	uint32_t DescCount = 0;
	spvReflectEnumerateDescriptorSets(&ShaderModule, &DescCount, nullptr);

	std::vector<SpvReflectDescriptorSet*> DescSets(DescCount);
	if (spvReflectEnumerateDescriptorSets(&ShaderModule, &DescCount, DescSets.data()) == SPV_REFLECT_RESULT_SUCCESS)
	{
		for (size_t DescSetIdx = 0; DescSetIdx < DescSets.size(); ++DescSetIdx) {

			const SpvReflectDescriptorSet& ReflectDescSet = *(DescSets[DescSetIdx]);

			for (uint32_t BindingIdx = 0; BindingIdx < ReflectDescSet.binding_count; ++BindingIdx) {

				const SpvReflectDescriptorBinding& ReflectBinding = *(ReflectDescSet.bindings[BindingIdx]);
				VkDescriptorSetLayoutBinding DescBinding = {};
				DescBinding.binding = ReflectBinding.binding;
				DescBinding.descriptorType = static_cast<VkDescriptorType>(ReflectBinding.descriptor_type);
				DescBinding.descriptorCount = 1;
				for (uint32_t i_dim = 0; i_dim < ReflectBinding.array.dims_count; ++i_dim) {
					DescBinding.descriptorCount *= ReflectBinding.array.dims[i_dim];
				}
				DescBinding.stageFlags = static_cast<VkShaderStageFlagBits>(ShaderModule.shader_stage);

				mDescriptorSet->AddBinding(DescBinding);
			}

		}
	}
}
}
