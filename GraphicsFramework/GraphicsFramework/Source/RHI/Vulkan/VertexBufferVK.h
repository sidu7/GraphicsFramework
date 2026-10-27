/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

#include "Rendering/VertexBuffer.h"
#include "RHI/Vulkan/VulkanTypes.h"

namespace rhi::vk
{
class VertexBufferVK : public VertexBuffer
{
public:
	VertexBufferVK();
	virtual ~VertexBufferVK();
	
	virtual void SetDataLayout(const std::vector<VertexFormat>& vertexFormats) override;
	virtual void Delete() override;

	virtual void AddData(const void* data,unsigned int size) override;
	virtual void AddSubData(const void* data, unsigned int size, unsigned offset = 0) override;
	virtual void AddDynamicData(const void* data, unsigned int size) override;

	static enum VkFormat GetVertexFormat(VertexFormat format);

	BufferInfo StagingBufferInfo;   // unused, kept for ABI/source compatibility
	BufferInfo MainBufferInfo;
protected:
	void EnsureCapacity(uint32_t requiredSize);
	void WriteAt(const void* data, unsigned int size, unsigned int offset);
};
}
