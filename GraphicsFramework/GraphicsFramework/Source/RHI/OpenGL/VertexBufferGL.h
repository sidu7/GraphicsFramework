/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

#include "Core/Core.h"
#include "Rendering/VertexBuffer.h"

namespace rhi::gl
{
class VertexBufferGL : public VertexBuffer
{
public:
	VertexBufferGL();
	virtual ~VertexBufferGL();
	
	virtual void SetDataLayout(const std::vector<VertexFormat>& vertexFormats) override;
	virtual void Delete() override;
	
	virtual void AddData(const void* data,unsigned int size) override;
	virtual void AddSubData(const void* data, unsigned int size, unsigned offset = 0) override;
	virtual void AddDynamicData(const void* data, unsigned int size) override;


	unsigned int m_VertexBufferID;
	unsigned int m_VertexArrayID;
};
}
