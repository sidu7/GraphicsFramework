/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

#include "Rendering/VertexBuffer.h"
#include "RenderingFactory_Vulkan.h"

class VertexBuffer_Vulkan : public VertexBuffer
{
public:
	VertexBuffer_Vulkan();
	virtual ~VertexBuffer_Vulkan();
	
	virtual void SetDataLayout(const std::vector<VertexFormat>& vertexFormats) override;
	virtual void Delete() const override;

	virtual void AddData(const void* data,unsigned int size) override;
	virtual void AddSubData(const void* data, unsigned int size, unsigned offset = 0) override;
	virtual void AddDynamicData(const void* data, unsigned int size) override;

	static enum VkFormat GetVertexFormat(VertexFormat format);

	BufferInfo StagingBufferInfo;
	BufferInfo MainBufferInfo;
protected:
	void CreateBufferInfo(uint32_t vertexDataSize);
};
